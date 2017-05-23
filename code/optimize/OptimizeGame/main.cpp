#include "StableHeaders.h"

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_MovementCalculationSystem.h"
#include "GO_MovementApplySystem.h"
#include "GO_CalculateCombatDamageSystem.h"
#include "GO_ApplyCombatDamageSystem.h"
#include "GO_ApplyEntityDeathsSystem.h"
#include "GO_ApplyEntitySpawnsSystem.h"
#include "GO_UpdateInputSystem.h"
#include "GO_DebugInputSystem.h"

#include "GO_EventBroker.h"
#include "GO_ThreadUtils.h"

#include "GO_AssertMutex.h"
#include "GO_World.h"
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
#include "GO_PlayerInputComponent.h"

#include "GO_PropagationMatrix.h"
#include "GO_PropagationRow.h"

#include "GO_UniformThreadGroup.h"

const int Int64TextWidth = 20;


GO::Gameworks ourGameworks;

sf::Time deltaTime;




template<typename T>
void AddSystemForUpdate(T& aSystemToUpdate, GO::TaskSchedulerNew& aTaskScheduler, GO::SystemUpdateParams& someUpdateParams)
{
	const GO::TaskIdentifiers taskIdentifier = GO::GameUpdateSystemTypeTraits<T>::ourTaskIdentifier;
	const unsigned char profilerTag = GO::ProfilerTypeTraits<T>::ourProfilerTag;

	std::vector<unsigned int> taskDependencies;
	GO::GameUpdateSystemTypeTraits<T>::ourTaskDependencies::GatherTaskDependencies(taskDependencies);

	GO::Task updateSystemTask(unsigned int(taskIdentifier), std::bind(RunGameUpdateSystem, &aSystemToUpdate, someUpdateParams), profilerTag);

	if(taskDependencies.size() == 0)
	{
		aTaskScheduler.addTask(updateSystemTask);
	}
	else if(taskDependencies.size() == 1)
	{
		aTaskScheduler.addTask(updateSystemTask, taskDependencies[0]);
	}
	else if(taskDependencies.size() == 2)
	{
		aTaskScheduler.addTask(updateSystemTask, taskDependencies[0], taskDependencies[1]);
	}
	else
	{
		GO_ASSERT(false, "Too many dependencies for current implementation");
	}
}


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

void ApplyEntitySpawns(const GO::SystemUpdateParams& someUpdateParams)
{

}


// An empty function used for the 'Noop' sync points in the task scheduler
void Noop()
{
}


void RunGameUpdateSystem(GO::GameUpdateSystem* aSystemToUpdate, GO::SystemUpdateParams& someUpdateParams)
{
	aSystemToUpdate->updateSystem(someUpdateParams);
}


void RunGame()
{
	TestFrameProcessor();

	ourGameworks.myDataDirectory = "..\\..\\..\\data\\";
	sf::err().rdbuf(ourGameworks.mySFMLOutput.rdbuf());

	sf::RenderWindow window(sf::VideoMode(1600, 900), "SFML works!", sf::Style::Close | sf::Style::Titlebar);
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
	playerEntity->createComponent<GO::PlayerInputComponent>();

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


	GO::CalculateCombatDamageSystem calcCombatDamageSystem;
	GO::MovementCalculationSystem calcEntityMovementSystem(world);
	GO::MovementApplySystem movementSystem(world);
	GO::ApplyCombatDamageSystem applyCombatDamageSystem;
	GO::ApplyEntityDeathsSystem applyEntityDeathsSystem;
	GO::ApplyEntitySpawnsSystem applyEntitySpawnsSystem;
	GO::UpdateInputSystem updateInputSystem;
	GO::DebugInputSystem debugInputSystem;

	window.setVerticalSyncEnabled(true);

	while(window.isOpen())
	{
		PROFILER_BEGIN_FRAME(testProfiler);

		{
			PROFILER_SCOPED(&testProfiler, "Main Loop", 0xffffffff);

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


			GO::TaskSchedulerNew  scheduler(threadPool, unsigned int(GO::TaskIdentifiers::MaxNumberTasks), &testProfiler);
			GO::SystemUpdateParams systemUpdateParams(world, testProfiler, threadPool, window, gameInstance, deltaTime);


			// Game Calculation Steps
			{
				PROFILER_SCOPED(&testProfiler, "Calculate Game Step", 0xff0000ff);
				AddSystemForUpdate(calcCombatDamageSystem, scheduler, systemUpdateParams);
				AddSystemForUpdate(calcEntityMovementSystem, scheduler, systemUpdateParams);
				AddSystemForUpdate(updateInputSystem, scheduler, systemUpdateParams);

				// TODO(scarroll): Make the dependency on the system (and it's type traits) rather than on the identifier within it
				GO::Task noopTask(unsigned int(GO::TaskIdentifiers::CalculateFinished), std::bind(Noop), GO_ProfilerTags::THREAD_TAG_CALC_SYNC_POINT_TASK);
				scheduler.addTask(noopTask, unsigned int(GO::TaskIdentifiers::CalculateCombat), unsigned int(GO::TaskIdentifiers::CalculateMovement), unsigned int(GO::TaskIdentifiers::UpdateInput));
			}

			// Game Apply Steps
			{
				PROFILER_SCOPED(&testProfiler, "Apply Game Step", 0x00ff00ff);

				AddSystemForUpdate(applyCombatDamageSystem, scheduler, systemUpdateParams);
				AddSystemForUpdate(applyEntityDeathsSystem, scheduler, systemUpdateParams);
				AddSystemForUpdate(applyEntitySpawnsSystem, scheduler, systemUpdateParams);
				AddSystemForUpdate(movementSystem, scheduler, systemUpdateParams);
				AddSystemForUpdate(debugInputSystem, scheduler, systemUpdateParams);
			}


			{
				PROFILER_SCOPED(&testProfiler, "Waiting", 0xFFFF00FF);
				scheduler.runPendingTasks();

				// TODO(scarroll): This is required because runPendingTasks does not store/reset when it changes the tag
				testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);
			}



			// Rendering
			// TODO(scarroll): Move this into it's own thread so that it can be constantly running
			//					and updating as fast as possible. It should not have data ties with
			//					the current game state
			{
				{
					PROFILER_SCOPED(&testProfiler, "Clear Window", 0x00ff00ff);
					window.clear();
				}

				{
					PROFILER_SCOPED(&testProfiler, "Render Sprites", 0x00ffffff);
					GO::World::EntityList& entityList = world.getEntities();
					for (size_t i = 0; i < entityList.size(); i++)
					{
						GO::Entity& currentEntity = *entityList[i];
						GO::SpriteComponent* spriteComponent = currentEntity.getComponent<GO::SpriteComponent>();
						if (spriteComponent)
						{
							spriteComponent->render(window);
						}
					}
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
				PROFILER_SCOPED(&testProfiler, "Profiler Rendering", 0xffff00ff);
				testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_PROFILER_RENDER);

				sf::Vector2u windowSize = window.getSize();

				sf::Vector2f profilerSize = sf::Vector2f(0.9f * windowSize.x, 0.9f * windowSize.y);
				sf::Vector2f profilerOffset = sf::Vector2f(0.05f * windowSize.x, 0.05f * windowSize.y);

				if (testProfiler.GetPreviousFrameStartTime())
				{
					GO_ProfilerRenderer::RenderProfiler(testProfiler, testProfiler.GetPreviousFrameCallGraphRoots(), profilerOffset, profilerSize, window);
				}

				// TODO(scarroll): The thread tag profiler display is disabled now that the full call graph display is available
				//					Investigate whether we can get a simplified version of the thread display from the call graph display
				//if(testProfiler.GetPreviousFrameStartTime())
				//{
				//	const std::vector<ProfilerThreadEvent>& threadEvents = testProfiler.GetPreviousFrameThreadEvents();
				//	sf::Vector2f threadProfilerOffset = sf::Vector2f(0.0f, 150.0f);

				//	GO_ProfilerRenderer::RenderThreadEvents(threadEvents, testProfiler.GetPreviousFrameStartTime(), testProfiler.GetPreviousFrameEndTime(), threadProfilerOffset, profilerSize, window);
				//	GO_ProfilerRenderer::RenderThreadEventTitles(threadEvents, sf::Vector2f(130.0f, 475.0f), window);
				//}
			}
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_MAIN_THREAD);


			{
				PROFILER_SCOPED(&testProfiler, "Rendering", 0xaa00aaff);
				window.display();
			}

			// TODO: We shouldn't have to insert this one once the final events for each thread are drawn
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
		}
		PROFILER_END_FRAME(testProfiler);
	}

	delete heapPropMatrix;
	heapPropMatrix = nullptr;
}