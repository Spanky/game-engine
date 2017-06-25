#include "StableHeaders.h"

#include "UpdatePlayerInputSystem.h"

#include "GO_Assert.h"
#include "GO_AssertMutex.h"

#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_ComponentAccessControl.h"
#include "GO_GameUpdateSystem.h"

#include "GO_TaskIdentifiers.h"
#include "GO_TaskSchedulerNew.h"

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


static_assert(sizeof(sf::Time) == sizeof(size_t), "Time is bigger than a native type and should be passed by reference");

static const int Int64TextWidth = 20;

static int ourGlobalFrameCounter = 0;
static GO::Gameworks ourGameworks;
static sf::Time deltaTime;


void RunGameUpdateSystem(GO::GameUpdateSystem* aSystemToUpdate, GO::SystemUpdateParams& someUpdateParams)
{
	aSystemToUpdate->updateSystem(someUpdateParams);
}

template<typename T>
void AddSystemForUpdate(T& aSystemToUpdate, GO::TaskSchedulerNew& aTaskScheduler, GO::SystemUpdateParams& someUpdateParams)
{
	const GO::TaskIdentifiers taskIdentifier = GO::GameUpdateSystemTypeTraits<T>::ourTaskIdentifier;
	const unsigned char profilerTag = GO::ProfilerTypeTraits<T>::ourProfilerTag;

	std::vector<unsigned int> taskDependencies;
	GO::GameUpdateSystemTypeTraits<T>::ourTaskDependencies::GatherTaskDependencies(taskDependencies);

	GO::Task updateSystemTask(unsigned int(taskIdentifier), std::bind(RunGameUpdateSystem, &aSystemToUpdate, someUpdateParams), profilerTag);

	if (taskDependencies.size() == 0)
	{
		aTaskScheduler.addTask(updateSystemTask);
	}
	else if (taskDependencies.size() == 1)
	{
		aTaskScheduler.addTask(updateSystemTask, taskDependencies[0]);
	}
	else if (taskDependencies.size() == 2)
	{
		aTaskScheduler.addTask(updateSystemTask, taskDependencies[0], taskDependencies[1]);
	}
	else
	{
		GO_ASSERT(false, "Too many dependencies for current implementation");
	}
}

// An empty function used for the 'Noop' sync points in the task scheduler
void Noop()
{
}




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


void RunGame()
{
	ourGameworks.myDataDirectory = "..\\..\\..\\data\\";
	sf::err().rdbuf(ourGameworks.mySFMLOutput.rdbuf());

	sf::RenderWindow window(sf::VideoMode(1600, 900), "SFML works!", sf::Style::Close | sf::Style::Titlebar);
	//window.setVerticalSyncEnabled(true);

	sf::Clock frameTimer;

	GO::GameInstance gameInstance(window, ourGameworks);
	GO::World world;

	GO::Entity* playerEntity = world.createEntity();
	//playerEntity->createComponent<GO::SpriteComponent>();
	//playerEntity->createComponent<GO::MovementComponent>();
	//playerEntity->createComponent<GO::HealthComponent>();
	playerEntity->createComponent<GO::PlayerInputComponent>();

	//for (int i = 0; i < 500; ++i)
	//{
	//	GO::Entity* entity = world.createEntity();
	//	entity->createComponent<GO::SpriteComponent>();
	//	entity->createComponent<GO::RandomMovementComponent>();
	//	entity->createComponent<GO::MovementComponent>();
	//	entity->createComponent<GO::HealthComponent>();
	//}


#if PROFILER_ENABLED
	GO_APIProfiler testProfiler;
#endif	// #if PROFILER_ENABLED

	GO::ThreadPool threadPool(&testProfiler, 0);

	Bricks::UpdatePlayerInputSystem updatePlayerInputSystem;

	window.setVerticalSyncEnabled(true);

	while (window.isOpen())
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
				AddSystemForUpdate(updatePlayerInputSystem, scheduler, systemUpdateParams);

				// TODO(scarroll): Make the dependency on the system (and it's type traits) rather than on the identifier within it
				GO::Task noopTask(unsigned int(GO::TaskIdentifiers::CalculateFinished), std::bind(Noop), GO_ProfilerTags::THREAD_TAG_CALC_SYNC_POINT_TASK);
				scheduler.addTask(noopTask, unsigned int(GO::TaskIdentifiers::UpdatePlayerInput));
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

				//{
				//	PROFILER_SCOPED(&testProfiler, "Render Sprites", 0x00ffffff);

				//	GO::ScopedComponentAccessFlags scopedAccessFlags = GO::ScopedComponentAccessFlags(GO::ReadComponentList<>(), GO::WriteComponentList<GO::SpriteComponent>());

				//	GO::World::EntityList& entityList = world.getEntities();
				//	for (size_t i = 0; i < entityList.size(); i++)
				//	{
				//		GO::Entity& currentEntity = *entityList[i];
				//		GO::SpriteComponent* spriteComponent = currentEntity.getComponentWriteAccess<GO::SpriteComponent>(scopedAccessFlags.getRights());
				//		if (spriteComponent)
				//		{
				//			spriteComponent->render(window);
				//		}
				//	}
				//}
			}

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
				PROFILER_SCOPED(&testProfiler, "Render Swap", 0xaa00aaff);
				window.display();
			}

			// TODO: We shouldn't have to insert this one once the final events for each thread are drawn
			testProfiler.PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
		}
		PROFILER_END_FRAME(testProfiler);
	}
}