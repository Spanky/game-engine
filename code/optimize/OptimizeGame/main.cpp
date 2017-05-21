#include "StableHeaders.h"

#include <algorithm>

#include "GO_MovementCalculationSystem.h"
#include "GO_MovementApplySystem.h"

#include "GO_AssertMutex.h"
#include "GO_World.h"
#include "GO_TaskScheduler.h"
#include "GO_TaskSchedulerNew.h"
#include "GO_Entity.h"
#include "GO_GameInstance.h"
#include "GO_Gameworks.h"
#include "GO_HacksGlobalResources.h"
#include "GO_ThreadPool.h"

#include "GO_Profiler.h"
#include "GO_ProfilerTags.h"
#include "GO_ProfilerRenderer.h"


#include "GO_MovementComponent.h"
#include "GO_HealthComponent.h"
#include "GO_StatusEffectComponent.h"
#include "GO_SpriteComponent.h"
#include "GO_RandomMovementComponent.h"

#include "GO_PropagationMatrix.h"
#include "GO_PropagationRow.h"

#include "GO_UniformThreadGroup.h"

const int Int64TextWidth = 20;


GO::Gameworks ourGameworks;

sf::Time deltaTime;


struct SystemUpdateParams
{
	SystemUpdateParams(GO::World& aWorld, GO_APIProfiler& aProfiler, GO::ThreadPool& aThreadPool)
		: myWorld(aWorld)
		, myProfiler(aProfiler)
		, myThreadPool(aThreadPool)
	{
	};

	GO::World& myWorld;
	GO_APIProfiler& myProfiler;
	GO::ThreadPool& myThreadPool;
};

GO::AssertMutex ourCombatDamageMutex;


static_assert(sizeof(sf::Time) == sizeof(size_t), "Time is bigger than a native type and should be passed by reference");


void drawFpsCounter(sf::RenderWindow& aWindow, const sf::Font& aFont, sf::Time aDeltaTime)
{
	sf::Text text;
	text.setFont(aFont);
	text.setColor(sf::Color::Red);

	char buffer[Int64TextWidth];
	_i64toa_s(aDeltaTime.asMicroseconds(), buffer, Int64TextWidth, 10);
	//_gcvt_s(buffer, Int64TextWidth, elapsedTime, 20);

	text.setString(buffer);

	aWindow.draw(text);
}


void drawTimer(sf::RenderWindow& aWindow, const sf::Font& aFont, sf::Time aDeltaTime, sf::Vector2f aPosition)
{
	sf::Text text;
	text.setFont(aFont);
	text.setColor(sf::Color::Red);
	text.setPosition(aPosition);

	char buffer[Int64TextWidth];
	_i64toa_s(aDeltaTime.asMicroseconds(), buffer, Int64TextWidth, 10);
	//_gcvt_s(buffer, Int64TextWidth, elapsedTime, 20);

	text.setString(buffer);

	aWindow.draw(text);
}

void drawThreadId(sf::RenderWindow& aWindow, const sf::Font& aFont)
{
	sf::Text text;
	text.setFont(aFont);

	std::thread::id threadId = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadId;
	text.setString(ss.str());
	text.setPosition(250.0f, 250.0f);

	aWindow.draw(text);
}


static int ourGlobalFrameCounter = 0;


enum class TaskState
{
	NotStarted,
	Started,
	Running,
	Waiting
};

class GO_FrameTaskProcessor
{
public:
	GO_FrameTaskProcessor(std::condition_variable* aFrameStartedEvent, std::mutex* aConditionMutex);

	void operator()();

private:
	std::condition_variable* myFrameStartedEvent;
	std::mutex* myConditionMutex;

	int myFrameLastUpdated;
	TaskState myState;
};


GO_FrameTaskProcessor::GO_FrameTaskProcessor(std::condition_variable* aFrameStartedEvent, std::mutex* aConditionMutex)
	: myFrameStartedEvent(aFrameStartedEvent)
	, myConditionMutex(aConditionMutex)
	, myFrameLastUpdated(0)
	, myState(TaskState::NotStarted)
{
	GO_ASSERT(myFrameStartedEvent, "Frame task processor requires a valid frame started event");
	GO_ASSERT(myConditionMutex, "Requires a mutex to lock for the condition variable");
}

void GO_FrameTaskProcessor::operator()()
{
	myState = TaskState::Started;

	while(true)
	{
		std::unique_lock<std::mutex> lk(*myConditionMutex);
		myState = TaskState::Waiting;
		myFrameStartedEvent->wait(lk, [this]
		{
			return myFrameLastUpdated != ourGlobalFrameCounter;
		});

		myState = TaskState::Running;

		myFrameLastUpdated = ourGlobalFrameCounter;
		if (ourGlobalFrameCounter == -1)
		{
			break;
		}
	}
}


void LaunchSampleGameThreads(GO::ThreadPool& aThreadPool, GO_APIProfiler& aProfiler)
{
	std::atomic_int counter = 0;
	const int NumJobsSubmitted = 200;

	{
		std::vector<std::future<int>> futures(NumJobsSubmitted);

		for (int i = 0; i < NumJobsSubmitted; ++i)
		{
			std::future<int> result = aThreadPool.Submit([&counter, &aProfiler]() -> int
			{
				// TODO: The thread tag should be automatically gathered from the same tag as the 'submit' event
				aProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_GAME_TASK);
				//std::this_thread::sleep_for(std::chrono::nanoseconds(10));
				return counter.fetch_add(1);
			});

			futures[i] = std::move(result);
		}

		std::for_each(futures.begin(), futures.end(), [](std::future<int>& aFuture)
		{
			aFuture.get();
		});
	}


	int resultCounter = counter.load();
	GO_ASSERT(resultCounter == NumJobsSubmitted, "Number of jobs processed was not the same as the number of jobs submitted");
}

void TestFrameProcessor()
{
	const int numHardwareThreads = std::thread::hardware_concurrency();
	const int numFrameTasks = numHardwareThreads;


	std::condition_variable frameStarted;
	std::mutex frameStartedLock;

	GO_FrameTaskProcessor** frameProcessors = new GO_FrameTaskProcessor*[numFrameTasks];

	// TODO: Having to create std::function objects really sucks. Find a cleaner way to pass
	// the threadable object to the constructor of the uniform thread group
	auto callableFrameProcessors = new std::function<void()>[numFrameTasks];
	for (int i = 0; i < numFrameTasks; i++)
	{
		frameProcessors[i] = new GO_FrameTaskProcessor(&frameStarted, &frameStartedLock);
		callableFrameProcessors[i] = std::function<void()>(*frameProcessors[i]);
	}

	GO::UniformThreadGroup threadGroup(numFrameTasks, callableFrameProcessors);

	{
		std::lock_guard<std::mutex> lk(frameStartedLock);
		ourGlobalFrameCounter = 1;
		frameStarted.notify_all();
	}

	{
		std::lock_guard<std::mutex> lk(frameStartedLock);
		ourGlobalFrameCounter = -1;
		frameStarted.notify_all();
	}



	threadGroup.join();

	for (int i = 0; i < numFrameTasks; i++)
	{
		delete frameProcessors[i];
	}
	delete[] frameProcessors;
	frameProcessors = nullptr;


	delete[] callableFrameProcessors;

	delete[] frameProcessors;
	frameProcessors = nullptr;
}


template<typename Iterator, typename Callback, typename Type = typename std::iterator_traits<Iterator>::value_type>
void RunParallel(Iterator anIterBegin, Iterator anIterEnd, GO::World &aWorld, GO::ThreadPool &aThreadPool, GO_APIProfiler &aProfiler, const unsigned char aThreadTag, Callback aCallback)
{
	if (anIterBegin == anIterEnd)
	{
		return;
	}

	// TODO(scarroll): Get the number of tasks available from the thread pool
	const unsigned int threadCount = std::thread::hardware_concurrency();

	size_t numElements = anIterEnd - anIterBegin;

	// We have at least one job for every thread. Otherwise we'll just do the processing ourselves
	if (numElements >= threadCount)
	{
		const size_t NumJobsPerThread = (numElements + threadCount - 1) / threadCount;

		std::atomic_size_t counter = 0;
		{
			std::vector<std::future<int>> futures(threadCount - 1);

			// Run the parallel batches
			for (size_t i = 0; i < threadCount - 1; ++i)
			{
				const size_t startIndex = i * NumJobsPerThread;
				const size_t endIndex = (i + 1) * NumJobsPerThread;

				std::future<int> result = aThreadPool.Submit([&counter, &aProfiler, startIndex, endIndex, anIterBegin, aThreadTag, aCallback]() -> int
				{
					// TODO: The thread tag should be automatically gathered from the same tag as the 'submit' event
					aProfiler.PushThreadEvent(aThreadTag);
					aCallback(anIterBegin.operator->(), startIndex, endIndex, aProfiler);

					const size_t numProcessed = endIndex - startIndex;

					// TODO(scarroll): Remove the counter for batch processing once the system is complete
					return int(counter.fetch_add(numProcessed));
				});

				futures[i] = std::move(result);
			}


			// Run the local batch
			{
				const size_t startIndex = (threadCount - 1) * NumJobsPerThread;
				const size_t numProcessed = numElements - startIndex;
				aCallback(anIterBegin.operator->(), startIndex, numElements, aProfiler);
				counter.fetch_add(numProcessed);
			}

			std::for_each(futures.begin(), futures.end(), [](std::future<int>& aFuture)
			{
				aFuture.get();
			});
		}


		size_t resultCounter = counter.load();
		GO_ASSERT(resultCounter == numElements, "Number of jobs processed was not the same as the number of jobs submitted");
	}
	else
	{
		// Run everything locally since it's a small batch anyways
		aCallback(anIterBegin.operator->(), 0, numElements, aProfiler);
	}
}


struct CombatDamageMessage
{
	GO::Entity* myDealerEntity;
	GO::Entity* myReceiverEntity;
	float myDamageDealt;
};

struct EntityDeathMessage
{
	// TODO: Replace these with handles eventually to avoid the constant lookups all the time
	size_t myDeadEntityId;
	size_t myKillerEntityId;
};

struct EntitySpawnMessage
{
};


std::vector<CombatDamageMessage> ourCombatDamageMessages;
std::vector<EntityDeathMessage> ourEntityDeathMessages;
std::vector<EntitySpawnMessage> ourEntitySpawnMessages;



float DistanceBetweenEntities(const GO::Entity* aLHS, const GO::Entity* aRHS)
{
	const sf::Vector2i& lhsPosition = aLHS->getPosition();
	const sf::Vector2i& rhsPosition = aRHS->getPosition();

	return std::sqrt(((lhsPosition.x - rhsPosition.x) * (lhsPosition.x - rhsPosition.x))
		+ ((lhsPosition.y - rhsPosition.y) * (lhsPosition.y - rhsPosition.y)));
}

namespace GameEvents
{
	std::mutex ourDamageQueueMutex;

	void QueueDamage(GO::Entity* aDealer, GO::Entity* aReceiver, const float aDamageDone)
	{
		std::lock_guard<std::mutex> lock(ourDamageQueueMutex);

		CombatDamageMessage msg;
		msg.myDealerEntity = aDealer;
		msg.myReceiverEntity = aReceiver;
		msg.myDamageDealt = aDamageDone;

		ourCombatDamageMessages.push_back(msg);
	}

	void QueueDamage(const std::vector<CombatDamageMessage>& someDamageMessages)
	{
		std::lock_guard<std::mutex> lock(ourDamageQueueMutex);
		ourCombatDamageMessages.insert(ourCombatDamageMessages.end(), someDamageMessages.begin(), someDamageMessages.end());
	}

	void QueueEntityForDeath(const GO::Entity* aDeadEntity, const GO::Entity* aKillerEntity)
	{
		EntityDeathMessage msg;
		msg.myDeadEntityId = aDeadEntity->getId();
		msg.myKillerEntityId = aKillerEntity->getId();

		ourEntityDeathMessages.push_back(msg);
	}

	void QueueEntityForSpawn()
	{
		EntitySpawnMessage msg;

		ourEntitySpawnMessages.push_back(msg);
	}
}

void CalculateEntityMovements(SystemUpdateParams& someUpdateParams)
{
	GO::MovementCalculationSystem movementCalcSystem(someUpdateParams.myWorld);
	movementCalcSystem.GenerateCalculations();
}

void CalculateCombatDamage(SystemUpdateParams& someUpdateParams)
{
	GO::AssertMutexLock lock(ourCombatDamageMutex);
	GO::World& world = someUpdateParams.myWorld;
	GO::ThreadPool& threadPool = someUpdateParams.myThreadPool;
	GO_APIProfiler& profiler = someUpdateParams.myProfiler;

	GO::World::EntityList& entityList = world.getEntities();
	const size_t entityListSize = entityList.size();

	RunParallel(entityList.begin(), entityList.end(), world, threadPool, profiler, GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK,
		[&](GO::Entity** someEntities, const size_t aStartIndex, const size_t anEndIndex, GO_APIProfiler& aProfiler)
	{
		const GO::World::EntityList& entityList = world.getEntities();
		const size_t entityListSize = entityList.size();

		std::vector<CombatDamageMessage> damageMessages;

		for (size_t outerIndex = aStartIndex; outerIndex < anEndIndex; outerIndex++)
		{
			GO::Entity* outerEntity = someEntities[outerIndex];
			GO_ASSERT(outerEntity, "Entity list contains a nullptr");

			for (size_t innerIndex = outerIndex + 1; innerIndex < entityListSize; innerIndex++)
			{
				GO::Entity* innerEntity = entityList[innerIndex];
				GO_ASSERT(innerEntity, "Entity list contains a nullptr");

				const float distance = DistanceBetweenEntities(outerEntity, innerEntity);
				if (distance < 100.0f)
				{
					{
						CombatDamageMessage msg;
						msg.myDealerEntity = outerEntity;
						msg.myReceiverEntity = innerEntity;
						msg.myDamageDealt = 1.0f;
						damageMessages.push_back(msg);
					}

					{
						CombatDamageMessage msg;
						msg.myDealerEntity = innerEntity;
						msg.myReceiverEntity = outerEntity;
						msg.myDamageDealt = 1.0f;
						damageMessages.push_back(msg);
					}
				}
			}
		}

		// TODO(scarroll): Perform the gather in parallel with other tasks
		GameEvents::QueueDamage(damageMessages);
	});
}

void ApplyCombatDamageInternal(const CombatDamageMessage* someCombatMessages, const size_t aStartIndex, const size_t anEndIndex, GO_APIProfiler& aProfiler)
{
	for (size_t currentIndex = aStartIndex; currentIndex < anEndIndex; currentIndex++)
	{
		const CombatDamageMessage& damageMsg = (someCombatMessages[currentIndex]);

		GO::Entity* dealerEntity = damageMsg.myDealerEntity;
		GO::Entity* receiverEntity = damageMsg.myReceiverEntity;

		GO::HealthComponent* dealerHealthComp = dealerEntity->getComponent<GO::HealthComponent>();
		GO::HealthComponent* receiverHealthComp = receiverEntity->getComponent<GO::HealthComponent>();

		GO_ASSERT(receiverHealthComp, "Receiver that took damage doesn't have a health component");

		//receiverHealthComp->myHealth -= damageMsg.myDamageDealt;
		//if(receiverHealthComp->myHealth <= 0.0f)
		//{
		//	GameEvents::QueueEntityForDeath(receiverEntity, dealerEntity);
		//}
	}
}

void ApplyCombatDamage(const SystemUpdateParams& someUpdateParams)
{
	GO::AssertMutexLock lock(ourCombatDamageMutex);
	GO::World& world = someUpdateParams.myWorld;
	GO::ThreadPool& threadPool = someUpdateParams.myThreadPool;
	GO_APIProfiler& profiler = someUpdateParams.myProfiler;

	RunParallel(ourCombatDamageMessages.begin(), ourCombatDamageMessages.end(), world, threadPool, profiler, GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK, &ApplyCombatDamageInternal);

	ourCombatDamageMessages.clear();
}

void ApplyEntityDeaths(const SystemUpdateParams& someUpdateParams)
{
	GO::World& world = someUpdateParams.myWorld;
	const GO::World::EntityList& entityList = world.getEntities();

	for(const EntityDeathMessage& deathMsg : ourEntityDeathMessages)
	{
		const size_t deadEntityId = deathMsg.myDeadEntityId;
		
		GO::World::EntityList::const_iterator deadIter = std::find_if(entityList.begin(), entityList.end(),
			[deadEntityId](const GO::Entity* anEntity)
		{
			return anEntity->getId() == deadEntityId;
		});

		// We may have already destroyed this entity so don't bother destroying it again
		// (Multiple things could have all killed this entity in the same frame)
		if(deadIter != entityList.end())
		{
			world.destroyEntity(*deadIter);
		}
	}

	ourEntityDeathMessages.clear();
}

void ApplyEntitySpawns(const SystemUpdateParams& someUpdateParams)
{
	GO::World& world = someUpdateParams.myWorld;

	for(const EntitySpawnMessage& spawnMsg : ourEntitySpawnMessages)
	{
		GO::Entity* entity = world.createEntity();
		entity->createComponent<GO::SpriteComponent>();
		entity->createComponent<GO::RandomMovementComponent>();
		entity->createComponent<GO::HealthComponent>();
	}

	ourEntitySpawnMessages.clear();
}

void ApplyEntityMovement(const SystemUpdateParams& someUpdateParams)
{
	GO::MovementApplySystem movementSystem(someUpdateParams.myWorld);
	movementSystem.ApplyPendingMovementChanges(deltaTime);
}




enum class TaskIdentifiers : unsigned int
{
	CalculateCombat,
	CalculateMovement,
	CalculateFinished,
	ApplyCombatDamage,
	ApplyEntityDeaths,
	ApplyEntitySpawns,
	ApplyEntityMovement,
	MaxNumberTasks
};

// An empty function used for the 'Noop' sync points in the task scheduler
void Noop()
{
}


void RunGame()
{
	TestFrameProcessor();

	ourGameworks.myDataDirectory = "..\\..\\..\\data\\";
	sf::err().rdbuf(ourGameworks.mySFMLOutput.rdbuf());

	sf::RenderWindow window(sf::VideoMode(1280, 720), "SFML works!", sf::Style::Close | sf::Style::Titlebar);
	//window.setVerticalSyncEnabled(true);


	sf::Clock frameTimer;


	GO::PropagationMatrix* heapPropMatrix = new GO::PropagationMatrix(GO::PropagationRow::MatrixDimensions, GO::PropagationRow::MatrixDimensions, 50.0f);
	//GO::PropagationMatrix propMatrix(GO::PropagationRow::MatrixDimensions, GO::PropagationRow::MatrixDimensions, 50.0f);
	GO::PropagationMatrix& propMatrix = *heapPropMatrix;

	//// Move right 1 unit
	//{
	//	propMatrix.setCenter(sf::Vector2f(150.0f, 150.0f));

	//	sf::Vector2i lowerLeft = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(10.0f, 10.0f));
	//	GO_ASSERT(lowerLeft == sf::Vector2i(0, 0), "Lower left did not map correctly");

	//	sf::Vector2i upperRight = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(290.0f, 290.0f));
	//	GO_ASSERT(upperRight == sf::Vector2i(2, 2), "Upper right did not map correctly");

	//	sf::Vector2i center = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(150.0f, 150.0f));
	//	GO_ASSERT(center == sf::Vector2i(1, 1), "Center point did not map correctly");
	//}

	//// Move right 1 unit
	//{
	//	propMatrix.setCenter(sf::Vector2f(250.0f, 150.0f));

	//	sf::Vector2i lowerLeft = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(110.0f, 10.0f));
	//	GO_ASSERT(lowerLeft == sf::Vector2i(1, 0), "Lower left did not map correctly");

	//	sf::Vector2i upperRight = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(390.0f, 290.0f));
	//	GO_ASSERT(upperRight == sf::Vector2i(0, 2), "Upper right did not map correctly");

	//	sf::Vector2i center = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(250.0f, 150.0f));
	//	GO_ASSERT(center == sf::Vector2i(2, 1), "Center point did not map correctly");
	//}

	//// Move right 1 unit
	//{
	//	propMatrix.setCenter(sf::Vector2f(350.0f, 150.0f));

	//	sf::Vector2i lowerLeft = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(210.0f, 10.0f));
	//	GO_ASSERT(lowerLeft == sf::Vector2i(2, 0), "Lower left did not map correctly");

	//	sf::Vector2i upperRight = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(490.0f, 290.0f));
	//	GO_ASSERT(upperRight == sf::Vector2i(1, 2), "Upper right did not map correctly");

	//	sf::Vector2i center = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(350.0f, 150.0f));
	//	GO_ASSERT(center == sf::Vector2i(0, 1), "Center point did not map correctly");
	//}

	//// Move left 1 unit
	//{
	//	propMatrix.setCenter(sf::Vector2f(250.0f, 150.0f));

	//	sf::Vector2i lowerLeft = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(110.0f, 10.0f));
	//	GO_ASSERT(lowerLeft == sf::Vector2i(1, 0), "Lower left did not map correctly");

	//	sf::Vector2i upperRight = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(390.0f, 290.0f));
	//	GO_ASSERT(upperRight == sf::Vector2i(0, 2), "Upper right did not map correctly");

	//	sf::Vector2i center = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(250.0f, 150.0f));
	//	GO_ASSERT(center == sf::Vector2i(2, 1), "Center point did not map correctly");
	//}

	//// Move left 1 unit (back to original layout)
	//{
	//	propMatrix.setCenter(sf::Vector2f(150.0f, 150.0f));

	//	sf::Vector2i lowerLeft = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(10.0f, 10.0f));
	//	GO_ASSERT(lowerLeft == sf::Vector2i(0, 0), "Lower left did not map correctly");

	//	sf::Vector2i upperRight = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(290.0f, 290.0f));
	//	GO_ASSERT(upperRight == sf::Vector2i(2, 2), "Upper right did not map correctly");

	//	sf::Vector2i center = propMatrix.mapPositionToGridCoordinates(sf::Vector2f(150.0f, 150.0f));
	//	GO_ASSERT(center == sf::Vector2i(1, 1), "Center point did not map correctly");
	//}




	propMatrix.resetGrid(sf::Vector2f(150.0f, 150.0f));

	sf::Vector2f playerPosition(150.0f, 150.0f);
	propMatrix.setCenter(playerPosition);
	propMatrix.addContribution(sf::Vector2f(250.0f, 250.0f));

	GO::GameInstance gameInstance(window, ourGameworks);
	GO::World world;

	GO::Entity* playerEntity = world.createEntity();
	playerEntity->createComponent<GO::SpriteComponent>();
	playerEntity->createComponent<GO::MovementComponent>();
	playerEntity->createComponent<GO::HealthComponent>();

	for (int i = 0; i < 500; ++i)
	{
		GO::Entity* entity = world.createEntity();
		entity->createComponent<GO::SpriteComponent>();
		entity->createComponent<GO::RandomMovementComponent>();
		entity->createComponent<GO::MovementComponent>();
		entity->createComponent<GO::HealthComponent>();
	}


#if PROFILER_ENABLED
	GO_APIProfiler testProfiler;
#endif	// #if PROFILER_ENABLED

	GO::ThreadPool threadPool(&testProfiler, 0);

	while(window.isOpen())
	{
		PROFILER_BEGIN_FRAME(testProfiler);

		testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);
		
		deltaTime = frameTimer.restart();

		double elapsedTime = static_cast<double>(deltaTime.asMicroseconds()) / 1000.0;

		{
			PROFILER_SCOPED(&testProfiler, "Windows Loop", 0x0000ffff);
			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
				{
					window.close();
				}
			}
		}

		{
			PROFILER_SCOPED(&testProfiler, "Clear Window", 0x00ff00ff);
			window.clear();
		}


		{
			PROFILER_SCOPED(&testProfiler, "Task Scheduler", 0xffff00ff);
			window.setActive(false);

			std::future<int> result = threadPool.Submit([&testProfiler, &gameInstance, &window]() -> int
			{
				testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_TASK_SCHEDULER);
				window.setActive(true);
				gameInstance.getTaskScheduler().update();
				window.setActive(false);
				testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_UNKNOWN);

				return 1;
			});

			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
			result.get();
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);

			window.setActive(true);
		}


		GO::TaskSchedulerNew  scheduler(threadPool, unsigned int(TaskIdentifiers::MaxNumberTasks), &testProfiler);
		SystemUpdateParams systemUpdateParams(world, testProfiler, threadPool);

		{
			PROFILER_SCOPED(&testProfiler, "Calculate Game Step", 0xff0000ff);
			
			GO::Task calcDamageTask(unsigned int(TaskIdentifiers::CalculateCombat), std::bind(CalculateCombatDamage, systemUpdateParams), GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK);
			scheduler.addTask(calcDamageTask);

			GO::Task calcMovementsTask(unsigned int(TaskIdentifiers::CalculateMovement), std::bind(CalculateEntityMovements, systemUpdateParams), GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK);
			scheduler.addTask(calcMovementsTask);

			GO::Task noopTask(unsigned int(TaskIdentifiers::CalculateFinished), std::bind(Noop), GO_ProfilerTags::THREAD_TAG_CALC_SYNC_POINT_TASK);
			scheduler.addTask(noopTask, unsigned int(TaskIdentifiers::CalculateCombat), unsigned int(TaskIdentifiers::CalculateMovement));
		}

		{
			PROFILER_SCOPED(&testProfiler, "Apply Game Step", 0x00ff00ff);

			{
				SystemUpdateParams systemUpdateParams(world, testProfiler, threadPool);
				
				GO::Task combatDamageTask(unsigned int(TaskIdentifiers::ApplyCombatDamage), std::bind(&ApplyCombatDamage, systemUpdateParams), GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK);
				scheduler.addTask(combatDamageTask, unsigned int(TaskIdentifiers::CalculateFinished));

				GO::Task entityDeathTask(unsigned int(TaskIdentifiers::ApplyEntityDeaths), std::bind(&ApplyEntityDeaths, systemUpdateParams), GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK);
				scheduler.addTask(entityDeathTask, unsigned int(TaskIdentifiers::ApplyCombatDamage));

				GO::Task entitySpawnTask(unsigned int(TaskIdentifiers::ApplyEntitySpawns), std::bind(&ApplyEntitySpawns, systemUpdateParams), GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK);
				scheduler.addTask(entitySpawnTask, unsigned int(TaskIdentifiers::ApplyEntityDeaths));

				GO::Task entityMovementTask(unsigned int(TaskIdentifiers::ApplyEntityMovement), std::bind(&ApplyEntityMovement, systemUpdateParams), GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK);
				scheduler.addTask(entityMovementTask, unsigned int(TaskIdentifiers::ApplyEntitySpawns));

				scheduler.runPendingTasks();

				// TODO(scarroll): This is required because runPendingTasks does not store/reset when it changes the tag
				testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);
			}
		}


		


		//propMatrix.debugDraw();
		//sf::Clock propagationTimer;

		//{
		//	PROFILER_SCOPED(&testProfiler, "Propagation", 0xff55d3ff);
		//	std::future<int> result = threadPool.Submit([&testProfiler, &propMatrix]() -> int
		//	{
		//		testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_PROPAGATION);
		//		propMatrix.updateCells();

		//		return 1;
		//	});

		//	testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
		//	result.get();
		//	testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);
		//}

		//sf::Time propagationUpdateTime = propagationTimer.getElapsedTime();
		//drawTimer(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont(), propagationUpdateTime, sf::Vector2f(500.0f, 500.0f));


		drawFpsCounter(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont(), deltaTime);
		//drawThreadId(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont());


		{
			PROFILER_SCOPED(&testProfiler, "Sample Tasks", 0xff0000ff);
			// TODO: This is not strictly wait time. It's ignoring the time it takes to queue up the jobs
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
			LaunchSampleGameThreads(threadPool, testProfiler);
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);
		}


		{
			PROFILER_SCOPED(&testProfiler, "Profiler Rendering", 0xffff00ff);
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_PROFILER_RENDER);

			ProfilerNode* previousFrameNode = testProfiler.GetPreviousFrameRootNode();
			if(previousFrameNode)
			{
				float frameTime = GO_APIProfiler::TicksToMilliseconds(previousFrameNode->myAccumulator);

				sf::Vector2u windowSize = window.getSize();
				sf::Vector2f profilerSize = sf::Vector2f(0.8f * windowSize.x, 0.1f * windowSize.y);
				sf::Vector2f profilerOffset = sf::Vector2f(0.1f * windowSize.x, 0.1f * windowSize.y);

				sf::RectangleShape background(profilerSize);
				background.setPosition(profilerOffset);
				background.setFillColor(sf::Color(0x44444411));
				window.draw(background);

				GO_ProfilerRenderer::RenderProfilerLevel(previousFrameNode, profilerOffset, profilerSize, window);
			
				
				const std::vector<ProfilerThreadEvent>& threadEvents = testProfiler.GetPreviousFrameThreadEvents();

				sf::Vector2f threadProfilerOffset = sf::Vector2f(0.1f * windowSize.x, 0.1f * windowSize.y);
				threadProfilerOffset += sf::Vector2f(0.0f, profilerSize.y + 150.0f);

				GO_ProfilerRenderer::RenderThreadEvents(threadEvents, testProfiler.GetPreviousFrameStartTime(), frameTime, threadProfilerOffset, profilerSize, window);
				GO_ProfilerRenderer::RenderThreadEventTitles(threadEvents, sf::Vector2f(130.0f, 475.0f), window);
			}
		}
		testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);
		

		{
			PROFILER_SCOPED(&testProfiler, "Rendering", 0xaa00aaff);
			window.display();
		}

		// TODO: We shouldn't have to insert this one once the final events for each thread are drawn
		testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
		PROFILER_END_FRAME(testProfiler);
	}

	delete heapPropMatrix;
	heapPropMatrix = nullptr;
}