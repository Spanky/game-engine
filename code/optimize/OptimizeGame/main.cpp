#include "StableHeaders.h"

#include <algorithm>

#include "GO_AssertMutex.h"
#include "GO_World.h"
#include "GO_TaskScheduler.h"
#include "GO_Entity.h"
#include "GO_GameInstance.h"
#include "GO_Gameworks.h"
#include "GO_HacksGlobalResources.h"
#include "GO_ThreadPool.h"
#include "GO_Profiler.h"


#include "GO_StatusEffectComponent.h"
#include "GO_SpriteComponent.h"
#include "GO_RandomMovementComponent.h"

#include "GO_PropagationMatrix.h"
#include "GO_PropagationRow.h"

#include "GO_UniformThreadGroup.h"

const int Int64TextWidth = 20;

const long long ourProfilerMergeTolerance = 1000;

const unsigned char THREAD_TAG_UNKNOWN = 0;
const unsigned char THREAD_TAG_WAITING = 1;

const unsigned char THREAD_TAG_TASK_SCHEDULER = 2;
const unsigned char THREAD_TAG_PROPAGATION = 3;

const unsigned char THREAD_TAG_GAME_TASK = 4;
const unsigned char THREAD_TAG_MAIN_THREAD = 5;

const unsigned char THREAD_TAG_PROFILER_RENDER = 6;

GO::Gameworks ourGameworks;


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
	const int NumJobsSubmitted = 2000;

	{
		std::vector<std::future<int>> futures(NumJobsSubmitted);

		for (int i = 0; i < NumJobsSubmitted; ++i)
		{
			std::future<int> result = aThreadPool.Submit([&counter, &aProfiler]() -> int
			{
				// TODO: The thread tag should be automatically gathered from the same tag as the 'submit' event
				aProfiler.PushThreadEvent(THREAD_TAG_GAME_TASK);
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

void RenderProfilerNode(long long aProfiledTime, const float aParentTotalTime, unsigned int aColor, const sf::Vector2f& aRegionSize, sf::Vector2f& aCurrentOffset, sf::RenderWindow& aWindow)
{
	const float childTime = GO_APIProfiler::TicksToMilliseconds(aProfiledTime);
	const float childRectangleWidth = aRegionSize.x * (childTime / aParentTotalTime);

	sf::RectangleShape profilerOutline(sf::Vector2f(childRectangleWidth, aRegionSize.y));
	profilerOutline.setPosition(sf::Vector2f(aCurrentOffset));
	aCurrentOffset.x += childRectangleWidth;

	profilerOutline.setFillColor(sf::Color(aColor));

	aWindow.draw(profilerOutline);
}

void RenderProfilerLevel(ProfilerNode* aLevelParentNode, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow)
{
	const float parentTotalTime = GO_APIProfiler::TicksToMilliseconds(aLevelParentNode->myAccumulator);
	sf::Vector2f currentOffset = aStartingPosition;

	std::vector<ProfilerNode*> reversedOrderChildren;
	// Render children nodes
	ProfilerNode* childNode = aLevelParentNode->myFirstChild;
	while (childNode)
	{
		reversedOrderChildren.push_back(childNode);
		childNode = childNode->mySibling;
	}

	while(reversedOrderChildren.size())
	{
		ProfilerNode* childNode = reversedOrderChildren.back();
		reversedOrderChildren.erase(reversedOrderChildren.end() - 1);

		RenderProfilerNode(childNode->myAccumulator, parentTotalTime, childNode->myColor, aRegionSize, currentOffset, aWindow);
		childNode = childNode->mySibling;
	}

	// Render a node that is the 'unknown' time that wasn't tracked for this level
	// Count up the time of the children
	long long totalTicks = 0;
	{
		ProfilerNode* childNode = aLevelParentNode->myFirstChild;
		while (childNode)
		{
			totalTicks += childNode->myAccumulator;
			childNode = childNode->mySibling;
		}
	}

	const long long unknownTime = aLevelParentNode->myAccumulator - totalTicks;
	const float unknwonRectangleWidth = aRegionSize.x * (unknownTime / parentTotalTime);

	RenderProfilerNode(unknownTime, parentTotalTime, 0xffffffff, aRegionSize, currentOffset, aWindow);
}

sf::Color locGetThreadTagColor(unsigned char aThreadTag)
{
	switch(aThreadTag)
	{
	// NOTE: This is the 'unknown' case and we should really be pushing something on here
	case THREAD_TAG_UNKNOWN:
		return sf::Color(0xffffffff);
	case THREAD_TAG_WAITING:
		return sf::Color(0x00000000);
	case THREAD_TAG_TASK_SCHEDULER:
		return sf::Color(0xffff00ff);
	case THREAD_TAG_PROPAGATION:
		return sf::Color(0x00ffffff);
	case THREAD_TAG_GAME_TASK:
		return sf::Color(0xff0000ff);
	case THREAD_TAG_MAIN_THREAD:
		return sf::Color(0x00ff00ff);
	case THREAD_TAG_PROFILER_RENDER:
		return sf::Color(0x0000ffff);
	default:
		return sf::Color(0xff00ffff);
	}
}

void RenderThreadEvents(const std::vector<ProfilerThreadEvent>& someThreadEvents, const long long aFrameStartTime, const float aFrameDuration, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow)
{
	GO_ASSERT(aFrameDuration != 0, "Previous frame must have a duration");

	struct ThreadSpecificEventInfo
	{
		long long myLastEventStartTime;
		unsigned int myThreadIndex;
		unsigned char myLastThreadTag;
	};

	std::vector<ThreadSpecificEventInfo> threadSpecificInfos;

	unsigned int numDrawn = 0;


	long long mainThreadTime = 0;
	int numMainThreads = 0;
	for(size_t i = someThreadEvents.size() - 1; i >= 0; i--)
	{
		// TODO: Hard-coded thread index here
		if(someThreadEvents[i].myThreadIndex == GetCurrentThreadIndex())
		{
			numMainThreads++;
			if(numMainThreads == 4)
			{
				mainThreadTime = someThreadEvents[i].myStartTime;
				break;
			}
		}
	}
	GO_ASSERT(mainThreadTime != 0, "Could not locate 4th last main thread event");

	for(const ProfilerThreadEvent& pte : someThreadEvents)
	{
		GO_ASSERT(pte.myStartTime >= aFrameStartTime, "Received an event that started before the frame did");

		// As we go over every event, keep track of the threads that we encounter so we know where to put the markers
		size_t lookupIndex = 0;
		for(lookupIndex = 0; lookupIndex < threadSpecificInfos.size(); lookupIndex++)
		{
			if(threadSpecificInfos[lookupIndex].myThreadIndex == pte.myThreadIndex)
			{
				break;
			}
		}

		// We couldn't find a thread specific info entry for this thread so we have to create a new one
		if(lookupIndex == threadSpecificInfos.size())
		{
			threadSpecificInfos.push_back(ThreadSpecificEventInfo());

			lookupIndex = threadSpecificInfos.size() - 1;

			ThreadSpecificEventInfo& newThreadInfo = threadSpecificInfos[lookupIndex];
			newThreadInfo.myThreadIndex = pte.myThreadIndex;
			newThreadInfo.myLastEventStartTime = aFrameStartTime;
			newThreadInfo.myLastThreadTag = THREAD_TAG_WAITING;
		}

		ThreadSpecificEventInfo& threadSpecific = threadSpecificInfos[lookupIndex];

		// Since we are only storing the starting time of each event, we can't draw it until we encounter
		// the next event on the time line.
		
		// Skip elements that are under our tolerance to avoid creating tons of small rectangles. Merge them
		// into one larger rectangle

		GO_ASSERT(pte.myThreadIndex == GetCurrentThreadIndex() || pte.myStartTime <= mainThreadTime || threadSpecific.myLastThreadTag <= 1, "An event went beyond the 'main' we are done event");
		if( !(pte.myThreadIndex == GetCurrentThreadIndex() || pte.myStartTime <= mainThreadTime || threadSpecific.myLastThreadTag <= 1))
		{
			int* test = nullptr;
			(*test) = 5;
		}

		// TODO: The start time may be before this frame has actually started.
		//		 This would indicate an event that was carried forward from the
		//		 previous frame
		const long long eventStartTime = std::max<long long>(0, threadSpecific.myLastEventStartTime - aFrameStartTime);
		const long long eventEndTime = pte.myStartTime - aFrameStartTime;

		const float relativeEventStartTime = GO_APIProfiler::TicksToMilliseconds(eventStartTime);
		//const float relativeEventEndTime = GO_APIProfiler::TicksToMilliseconds(eventEndTime);

		const long long eventDuration = eventEndTime - eventStartTime;
		GO_ASSERT(eventDuration >= 0, "The event took 'negative' time");

		// Skip over any that can be merged together
		if(pte.myThreadTag == threadSpecific.myLastThreadTag &&
			(pte.myStartTime - threadSpecific.myLastEventStartTime) < ourProfilerMergeTolerance)
		{
			continue;
		}

		sf::Vector2f rectSize;
		rectSize.x = aRegionSize.x * (GO_APIProfiler::TicksToMilliseconds(eventDuration) / aFrameDuration);
		rectSize.y = 10.0f;

		sf::Vector2f rectPosition;
		rectPosition.x = aStartingPosition.x + (aRegionSize.x * (relativeEventStartTime / aFrameDuration));
		rectPosition.y = aStartingPosition.y + (threadSpecific.myThreadIndex * 15.0f);

		GO_ASSERT(rectPosition.x >= aStartingPosition.x, "The event will be drawn before the timeline");
		GO_ASSERT(rectSize.x >= 0.0f, "The event will be drawn with negative size");
		GO_ASSERT(rectPosition.x <= aStartingPosition.x + aRegionSize.x, "Bar goes beyond the bounds given to us");

		sf::RectangleShape profilerOutline(rectSize);
		profilerOutline.setPosition(rectPosition);

		sf::Color eventColor = locGetThreadTagColor(threadSpecific.myLastThreadTag);
		profilerOutline.setFillColor(eventColor);

		aWindow.draw(profilerOutline);
		numDrawn++;

		threadSpecific.myLastEventStartTime = pte.myStartTime;
		threadSpecific.myLastThreadTag = pte.myThreadTag;
	}

	if(numDrawn == 4123212313)
	{
		int* temp = nullptr;
		(*temp) = 5;
	}

	// TODO: Render the last events for each of the threads
	for(size_t lookupIndex = 0; lookupIndex < threadSpecificInfos.size(); lookupIndex++)
	{
		ThreadSpecificEventInfo& threadSpecific = threadSpecificInfos[lookupIndex];
		if(threadSpecific.myLastThreadTag <= 1)
		{
			continue;
		}

		//	// Since we are only storing the starting time of each event, we can't draw it until we encounter
		//	// the next event on the time line.

		const float endFrameTime = GO_APIProfiler::TicksToMilliseconds(aFrameStartTime) + aFrameDuration;
		const float eventDuration = endFrameTime - GO_APIProfiler::TicksToMilliseconds(threadSpecific.myLastEventStartTime);

		// TODO: The start time may be before this frame has actually started.
		//		 This would indicate an event that was carried forward from the
		//		 previous frame
		//const long long eventStartTime = std::max<long long>(0, threadSpecific.myLastEventStartTime - aFrameStartTime);
		//const long long eventEndTime =  - aFrameStartTime;

		const float relativeEventStartTime = GO_APIProfiler::TicksToMilliseconds(threadSpecific.myLastEventStartTime);
		//const float relativeEventEndTime = GO_APIProfiler::TicksToMilliseconds(eventEndTime);

		//const long long eventDuration = eventEndTime - eventStartTime;
		GO_ASSERT(eventDuration >= 0, "The event took 'negative' time");

		// Skip over any that can be merged together
		//if (pte.myThreadTag == threadSpecific.myLastThreadTag &&
		//	(pte.myStartTime - threadSpecific.myLastEventStartTime) < ourProfilerMergeTolerance)
		//{
		//	continue;
		//}

		sf::Vector2f rectSize;
		rectSize.x = aRegionSize.x * (eventDuration / aFrameDuration);
		rectSize.y = 10.0f;

		sf::Vector2f rectPosition;
		rectPosition.x = aStartingPosition.x + (aRegionSize.x * (relativeEventStartTime / aFrameDuration));
		rectPosition.y = aStartingPosition.y + (threadSpecific.myThreadIndex * 15.0f);

		GO_ASSERT(rectPosition.x >= aStartingPosition.x, "The event will be drawn before the timeline");
		GO_ASSERT(rectSize.x >= 0.0f, "The event will be drawn with negative size");
		GO_ASSERT(rectPosition.x <= aStartingPosition.x + aRegionSize.x, "Bar goes beyond the bounds given to us");

		sf::RectangleShape profilerOutline(rectSize);
		profilerOutline.setPosition(rectPosition);

		sf::Color eventColor = locGetThreadTagColor(threadSpecific.myLastThreadTag);
		profilerOutline.setFillColor(eventColor);

		aWindow.draw(profilerOutline);
		numDrawn++;
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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

	for (int i = 0; i < 5; ++i)
	{
		GO::Entity* entity = world.createEntity();
		entity->createComponent<GO::SpriteComponent>();
		entity->createComponent<GO::RandomMovementComponent>();
	}


	bool canMoveRight = true;
	bool canMoveLeft = true;

	bool canMoveUp = true;
	bool canMoveDown = true;

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

#if PROFILER_ENABLED
	GO_APIProfiler testProfiler;
#endif	// #if PROFILER_ENABLED

	GO::ThreadPool threadPool(&testProfiler, 0);

	while(window.isOpen())
	{
		PROFILER_BEGIN_FRAME(testProfiler);

		testProfiler.PushThreadEvent(THREAD_TAG_MAIN_THREAD);
		
		sf::Time deltaTime = frameTimer.restart();

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
				testProfiler.PushThreadEvent(THREAD_TAG_TASK_SCHEDULER);
				window.setActive(true);
				gameInstance.getTaskScheduler().update();
				window.setActive(false);

				return 1;
			});

			testProfiler.PushThreadEvent(THREAD_TAG_WAITING);
			result.get();
			testProfiler.PushThreadEvent(THREAD_TAG_MAIN_THREAD);

			window.setActive(true);
		}


		if (canMoveRight)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				canMoveRight = false;

				playerPosition += sf::Vector2f(100.0f, 0.0f);
				propMatrix.setCenter(playerPosition);
			}
		}
		else
		{
			if(!sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				canMoveRight = true;
			}
		}

		if (canMoveLeft)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				canMoveLeft = false;

				playerPosition -= sf::Vector2f(100.0f, 0.0f);
				propMatrix.setCenter(playerPosition);
			}
		}
		else
		{
			if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				canMoveLeft = true;
			}
		}



		if (canMoveDown)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				canMoveDown = false;

				playerPosition += sf::Vector2f(0.0f, 100.0f);
				propMatrix.setCenter(playerPosition);
			}
		}
		else
		{
			if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				canMoveDown = true;
			}
		}

		if (canMoveUp)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			{
				canMoveUp = false;

				playerPosition -= sf::Vector2f(0.0f, 100.0f);
				propMatrix.setCenter(playerPosition);
			}
		}
		else
		{
			if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			{
				canMoveUp = true;
			}
		}


		//propMatrix.debugDraw();

		sf::Clock propagationTimer;

		{
			PROFILER_SCOPED(&testProfiler, "Propagation", 0xff55d3ff);
			std::future<int> result = threadPool.Submit([&testProfiler, &propMatrix]() -> int
			{
				testProfiler.PushThreadEvent(THREAD_TAG_PROPAGATION);
				propMatrix.updateCells();

				return 1;
			});

			testProfiler.PushThreadEvent(THREAD_TAG_WAITING);
			result.get();
			testProfiler.PushThreadEvent(THREAD_TAG_MAIN_THREAD);
		}

		sf::Time propagationUpdateTime = propagationTimer.getElapsedTime();
		drawTimer(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont(), propagationUpdateTime, sf::Vector2f(500.0f, 500.0f));


		drawFpsCounter(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont(), deltaTime);
		//drawThreadId(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont());


		{
			PROFILER_SCOPED(&testProfiler, "Sample Tasks", 0xff0000ff);
			// TODO: This is not strictly wait time. It's ignoring the time it takes to queue up the jobs
			testProfiler.PushThreadEvent(THREAD_TAG_WAITING);
			LaunchSampleGameThreads(threadPool, testProfiler);
			testProfiler.PushThreadEvent(THREAD_TAG_MAIN_THREAD);
		}


		{
			PROFILER_SCOPED(&testProfiler, "Profiler Rendering", 0xffff00ff);
			testProfiler.PushThreadEvent(THREAD_TAG_PROFILER_RENDER);

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

				RenderProfilerLevel(previousFrameNode, profilerOffset, profilerSize, window);
			
				
				const std::vector<ProfilerThreadEvent>& threadEvents = testProfiler.GetPreviousFrameThreadEvents();

				sf::Vector2f threadProfilerOffset = sf::Vector2f(0.1f * windowSize.x, 0.1f * windowSize.y);
				threadProfilerOffset += sf::Vector2f(0.0f, profilerSize.y + 150.0f);

				RenderThreadEvents(threadEvents, testProfiler.GetPreviousFrameStartTime(), frameTime, threadProfilerOffset, profilerSize, window);
			}
		}
		testProfiler.PushThreadEvent(THREAD_TAG_MAIN_THREAD);
		

		{
			PROFILER_SCOPED(&testProfiler, "Rendering", 0xaa00aaff);
			window.display();
		}

		// TODO: We shouldn't have to insert this one once the final events for each thread are drawn
		testProfiler.PushThreadEvent(THREAD_TAG_WAITING);
		PROFILER_END_FRAME(testProfiler);
	}

	delete heapPropMatrix;
	heapPropMatrix = nullptr;

	return 0;
}