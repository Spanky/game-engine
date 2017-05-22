#include "StableHeaders.h"
#include "GO_Profiler.h"

#if PROFILER_ENABLED

// TODO: Move this to a common 'standards' file
static const int CacheLineSizeInBytes = 64;

static_assert(sizeof(CallGraphNode) <= CacheLineSizeInBytes, "CallGraphNode needs to fit in a single cache line");
static_assert((CacheLineSizeInBytes % sizeof(CallGraphNode)) == 0, "CallGraphNode needs to fit evenly in a cache line");

double GO_APIProfiler::ourFrequency = 0;

thread_local unsigned char locMyThreadIndex = MaxThreadCount;

static std::atomic_uchar locTotalThreadCount = 0;
static std::mutex locEventListMutex;




struct CallGraphHistory
{
	std::array<GO_APIProfiler::FrameCallGraphRoots, 30> myFrameHistory;
};

int locCurrentFrameIndex = 0;
int locViewFrameIndex = -1;
CallGraphHistory locCallGraphHistory;

GO_APIProfiler::FrameCallGraphRoots locFramePausedTempStorage = {};

GO_APIProfiler::FrameCallGraphRoots locThreadCallGraphNodeCurrents = {};


unsigned char GetCurrentThreadIndex()
{
	if (locMyThreadIndex == MaxThreadCount)
	{
		// TODO: The mod here is only to support our usage of std::thread since we spin up new threads
		//		 for every thread pool right now
		locMyThreadIndex = (locTotalThreadCount++);
		GO_ASSERT(locMyThreadIndex < MaxThreadCount, "Number of threads created exceeds hard-coded limits");
	}

	return locMyThreadIndex;
}

namespace
{
	long long locGetCurrentProfilerTime()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		return currentTime.QuadPart;
	}

	void ResetNode(CallGraphNode& aNode)
	{
		aNode.myName = nullptr;
		aNode.myStartTime = 0;
		aNode.myEndTime = 0;
		aNode.myColor = 0;

		aNode.myLastChild = nullptr;
		aNode.myPrevSibling = nullptr;
		aNode.myParent = nullptr;
	}

	void ResetToRootNode(CallGraphNode& aNode)
	{
		ResetNode(aNode);
		aNode.myName = "Root";
	}

	CallGraphNode* GetNewCallGraphNode()
	{
		CallGraphNode* callGraphNode = new CallGraphNode();
		ResetNode(*callGraphNode);

		return callGraphNode;
	}
}

GO_APIProfiler::GO_APIProfiler()
	: myPreviousFrameStartTime(0)
	, myPreviousFrameEndTime(0)
	, myCurrentFrameStartTime(0)
	, myCurrentFrameEndTime(0)
	, myProfilerOverhead(0)
	, myIsCollectionActive(true)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	ourFrequency = 1.0 / freq.QuadPart;

	for (int i = 0; i < locCallGraphHistory.myFrameHistory.size(); i++)
	{
		for(int j = 0; j < locCallGraphHistory.myFrameHistory[i].size(); j++)
		{
			locCallGraphHistory.myFrameHistory[i][j] = nullptr;
		}
	}

	for(int i = 0; i < MaxThreadCount; i++)
	{
		locThreadCallGraphNodeCurrents[i] = nullptr;
	}

	// TODO: Why do we need this memory barrier? It is a legacy call from the original Jeff Phreshing implementation
	MemoryBarrier();
}

void GO_APIProfiler::StoreFrameInHistory()
{
	myPreviousFrameStartTime = myCurrentFrameStartTime;
	myPreviousFrameEndTime = myCurrentFrameEndTime;

	myCurrentFrameStartTime = locGetCurrentProfilerTime();
	myCurrentFrameEndTime = 0;


	if(myIsCollectionActive)
	{
		locCurrentFrameIndex = (locCurrentFrameIndex + 1) % locCallGraphHistory.myFrameHistory.size();
		if (locViewFrameIndex != -1)
		{
			locViewFrameIndex = (locViewFrameIndex + 1) % locCallGraphHistory.myFrameHistory.size();
		}

		// Release the frame that is about to be overwritten with this frames profiling data
		for (int threadIndex = 0; threadIndex < MaxThreadCount; threadIndex++)
		{
			if (locCallGraphHistory.myFrameHistory[locCurrentFrameIndex][threadIndex])
			{
				ReleaseNodeHierarchy(locCallGraphHistory.myFrameHistory[locCurrentFrameIndex][threadIndex]);
				locCallGraphHistory.myFrameHistory[locCurrentFrameIndex][threadIndex] = nullptr;
			}
		}
	}
	else
	{
		// Release the frame that is about to be overwritten with this frames profiling data
		for (int threadIndex = 0; threadIndex < MaxThreadCount; threadIndex++)
		{
			if (locFramePausedTempStorage[threadIndex])
			{
				ReleaseNodeHierarchy(locFramePausedTempStorage[threadIndex]);
				locFramePausedTempStorage[threadIndex] = nullptr;
			}
		}
	}

	{
		std::lock_guard<std::mutex> eventLock(locEventListMutex);
		myPreviousThreadEvents = myThreadEvents;
		myThreadEvents.clear();
	}
}

const GO_APIProfiler::FrameCallGraphRoots& GO_APIProfiler::GetPreviousFrameCallGraphRoots() const
{
	// The current 'view' index is set to 'active' which means that the most recent frame should be
	// returned
	int prevFrame = 0;

	if(locViewFrameIndex == -1 || locViewFrameIndex == locCurrentFrameIndex)
	{
		prevFrame = (locCurrentFrameIndex - 1) % locCallGraphHistory.myFrameHistory.size();
	}
	else
	{
		prevFrame = locViewFrameIndex;
	}
	
	GO_ASSERT(prevFrame >= 0 && prevFrame < locCallGraphHistory.myFrameHistory.size(), "Previous frame index was incorrectly calculated");
	return locCallGraphHistory.myFrameHistory[prevFrame];
}


void GO_APIProfiler::BeginFrame()
{
	GO::AssertMutexLock lock(myProfilerEventMutex);
	StoreFrameInHistory();

	myProfilerOverhead = 0;

	const long long currentTime = locGetCurrentProfilerTime();
	myCurrentFrameStartTime = currentTime;

	if(myIsCollectionActive)
	{
		for (int i = 0; i < MaxThreadCount; i++)
		{
			CallGraphNode* threadRoot = GetNewCallGraphNode();
			ResetToRootNode(*threadRoot);

			threadRoot->myStartTime = currentTime;

			locThreadCallGraphNodeCurrents[i] = threadRoot;
			locCallGraphHistory.myFrameHistory[locCurrentFrameIndex][i] = threadRoot;
		}
	}
	else
	{
		for (int i = 0; i < MaxThreadCount; i++)
		{
			CallGraphNode* threadRoot = GetNewCallGraphNode();
			ResetToRootNode(*threadRoot);

			threadRoot->myStartTime = currentTime;

			locThreadCallGraphNodeCurrents[i] = threadRoot;
			locFramePausedTempStorage[i] = threadRoot;
		}
	}
}

void GO_APIProfiler::EndFrame()
{
	GO::AssertMutexLock lock(myProfilerEventMutex);

	// TODO: Calling locGetCurrentProfilerTime() an additional jmp and mov instruction
	//		 unrelated to the function call (it is inlined) compared to using:
	//		 LARGE_INTEGER currentTime;
	//		 QueryPerformanceCounter(&currentTime);
	//
	//		 And using the QuadPart directly.
	//
	//		 The jmp is related to the 'Flush' call. Figure out why this is happening.
	//
	//		 To be fair, the extra jmp and mov are inside of the if which is a rarely
	//		 executed branch so the initial if-jmp will likely skip it
	//
	//		The if-jmp moves forward 10 bytes in the example above where as it moves
	//		forward 15 bytes in the code below
	//
	//		long long endTime = locGetCurrentProfilerTime();
	//
	//		However, if we store the value in a member, we only get an extra mov
	//		to store it in the instance and there is no additional jmp created
	// ----------------------------------------------------------------------------
	
	long long frameEndTime = locGetCurrentProfilerTime();

	myCurrentFrameEndTime = frameEndTime;

	for (int i = 0; i < MaxThreadCount; i++)
	{
		locThreadCallGraphNodeCurrents[i]->myEndTime = frameEndTime;
		GO_ASSERT(locThreadCallGraphNodeCurrents[i]->myParent == nullptr, "Some nodes were not popped correctly")
	}
}

void GO_APIProfiler::ReleaseNodeHierarchy(CallGraphNode* aCallGraphNode)
{
	CallGraphNode* childNode = aCallGraphNode->myLastChild;
	while (childNode)
	{
		CallGraphNode* nextChild = childNode->myPrevSibling;
		ReleaseNodeHierarchy(childNode);

		childNode = nextChild;
	}

	delete aCallGraphNode;
}

void GO_APIProfiler::BeginProfile(const char* aName, unsigned int aColor)
{
	//GO::AssertMutexLock lock(myProfilerEventMutex);
	GO_ASSERT(locGetCurrentProfilerTime() >= myCurrentFrameStartTime, "Recording a thread profile time before the frame has actually started");

	long long startOverheadTime = locGetCurrentProfilerTime();

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	const long long currentTime = start.QuadPart;


	unsigned char currentThreadIndex = GetCurrentThreadIndex();

	// We need a new root node for this thread
	CallGraphNode* parentNode = locThreadCallGraphNodeCurrents[currentThreadIndex];

	CallGraphNode* newCallNode = GetNewCallGraphNode();
	newCallNode->myPrevSibling = parentNode->myLastChild;
	newCallNode->myName = aName;
	newCallNode->myColor = aColor;
	newCallNode->myStartTime = currentTime;
	newCallNode->myParent = parentNode;

	parentNode->myLastChild = newCallNode;

	locThreadCallGraphNodeCurrents[currentThreadIndex] = newCallNode;


	QueryPerformanceCounter(&start);
	myProfilerOverhead += (start.QuadPart - startOverheadTime);
}

void GO_APIProfiler::EndProfile(const char* aName)
{
	//GO::AssertMutexLock lock(myProfilerEventMutex);

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	long long profileEndTime = start.QuadPart;


	unsigned char currentThreadIndex = GetCurrentThreadIndex();

	// We need a new root node for this thread
	CallGraphNode* currentNode = locThreadCallGraphNodeCurrents[currentThreadIndex];
	GO_ASSERT(currentNode->myParent != nullptr, "Attempting to remove a call graph node that has no parent");
	currentNode->myEndTime = profileEndTime;

	GO_ASSERT(currentNode->myName == aName, "EndProfile called on a mismatched tag");

	locThreadCallGraphNodeCurrents[currentThreadIndex] = currentNode->myParent;


	QueryPerformanceCounter(&start);
	myProfilerOverhead += (start.QuadPart - profileEndTime);
}

void GO_APIProfiler::PushThreadEvent(unsigned char aThreadTag)
{
	ProfilerThreadEvent evt;
	evt.myStartTime = locGetCurrentProfilerTime();
	evt.myThreadIndex = GetCurrentThreadIndex();
	evt.myThreadTag = aThreadTag;

	GO_ASSERT(evt.myStartTime >= myCurrentFrameStartTime, "Recording a thread profile time before the frame has actually started");

	{
		std::lock_guard<std::mutex> lock(locEventListMutex);
		myThreadEvents.push_back(evt);
	}
}

float GO_APIProfiler::TicksToMilliseconds(long long aTickCount)
{
	return aTickCount * ourFrequency * 1000;
}

void GO_APIProfiler::PauseCollection()
{
	myIsCollectionActive = false;
}

void GO_APIProfiler::ResumeCollection()
{
	myIsCollectionActive = true;
}

void GO_APIProfiler::ViewPrevFrame()
{
	locViewFrameIndex = (locViewFrameIndex - 1) % locCallGraphHistory.myFrameHistory.size();
}

void GO_APIProfiler::ViewNextFrame()
{
	locViewFrameIndex = (locViewFrameIndex + 1) % locCallGraphHistory.myFrameHistory.size();
}

#endif