#include "StableHeaders.h"
#include "GO_Profiler.h"

#if PROFILER_ENABLED

// TODO: Move this to a common 'standards' file
static const int CacheLineSizeInBytes = 64;

static_assert(sizeof(ProfilerNode) <= CacheLineSizeInBytes, "ProfilerNode needs to fit in a single cache line");
static_assert((CacheLineSizeInBytes % sizeof(ProfilerNode)) == 0, "ProfilerNode needs to fit evenly in a cache line");

double GO_APIProfiler::ourFrequency = 0;

thread_local unsigned char locMyThreadIndex = UCHAR_MAX;

static std::atomic_uchar locTotalThreadCount = 0;
static std::mutex locEventListMutex;


unsigned char GetCurrentThreadIndex()
{
	if (locMyThreadIndex == UCHAR_MAX)
	{
		// TODO: The mod here is only to support our usage of std::thread since we spin up new threads
		//		 for every thread pool right now
		locMyThreadIndex = (locTotalThreadCount++);
		GO_ASSERT(locMyThreadIndex < UCHAR_MAX, "Number of threads created exceeds hard-coded limits");
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

	void ResetNode(ProfilerNode& aNode)
	{
		aNode.myFirstChild = nullptr;
		aNode.myName = nullptr;
		aNode.mySibling = nullptr;
		aNode.myParent = nullptr;
		aNode.myCurrentStartTime = 0;
		aNode.myAccumulator = 0;
		aNode.myHitCount = 0;
	}

	void ResetToRootNode(ProfilerNode& aNode)
	{
		ResetNode(aNode);
		aNode.myName = "Root";
	}

	ProfilerNode* FindChildNode(ProfilerNode* aNode, const char* aChildName)
	{
		ProfilerNode* childNode = aNode->myFirstChild;
		while(childNode)
		{
			if(childNode->myName == aChildName)
			{
				return childNode;
			}
			else if (ProfilerNode* foundNode = FindChildNode(childNode, aChildName))
			{
				return foundNode;
			}

			childNode = childNode->mySibling;
		}

		return nullptr;
	}
}

GO_APIProfiler::GO_APIProfiler()
	: myStart(0)
	, myPreviousFrameStartTime(0)
	, myProfilerOverhead(0)
	, myCurrentNode(nullptr)
	, myPreviousFrameRootNode(nullptr)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	ourFrequency = 1.0 / freq.QuadPart;

	// TODO: Why do we need this memory barrier? It is a legacy call from the original Jeff Phreshing implementation
	MemoryBarrier();

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	myStart = start.QuadPart;
}

void GO_APIProfiler::StoreFrameInHistory()
{
	myPreviousFrameStartTime = myCurrentFrameStartTime;
	myCurrentFrameStartTime = locGetCurrentProfilerTime();

	if (myPreviousFrameRootNode)
	{
		ReleaseNodeHierarchy(myPreviousFrameRootNode);
	}
	myPreviousFrameRootNode = myCurrentNode;

	{
		std::lock_guard<std::mutex> eventLock(locEventListMutex);
		myPreviousThreadEvents = myThreadEvents;
		myThreadEvents.clear();
	}
}


void GO_APIProfiler::BeginFrame()
{
	StoreFrameInHistory();

	myCurrentNode = GetNewNode();
	ResetToRootNode(*myCurrentNode);

	myCurrentNode->myCurrentStartTime = locGetCurrentProfilerTime();
	myProfilerOverhead = 0;

	myCurrentFrameStartTime = myCurrentNode->myCurrentStartTime;
}

void GO_APIProfiler::EndFrame()
{
	assert(myCurrentNode->myParent == nullptr && "Profile begin/end pairs mismatched. Root node expected to be the current node");
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

	myCurrentNode->myAccumulator += (frameEndTime - myCurrentNode->myCurrentStartTime);
	myCurrentNode->myHitCount++;
}

void GO_APIProfiler::ReleaseNodeHierarchy(ProfilerNode* aProfilerNode)
{
	ProfilerNode* childNode = aProfilerNode->myFirstChild;
	while (childNode)
	{
		ProfilerNode* nextChild = childNode->mySibling;
		ReleaseNodeHierarchy(childNode);

		childNode = nextChild;
	}

	delete aProfilerNode;
}

ProfilerNode* GO_APIProfiler::GetNewNode()
{
	ProfilerNode* profilerNode = new ProfilerNode();
	ResetNode(*profilerNode);

	return profilerNode;
}

void GO_APIProfiler::BeginProfile(const char* aName, unsigned int aColor)
{
	long long startOverheadTime = locGetCurrentProfilerTime();

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);

	// Find an existing node from the current node for this new profiler block
	ProfilerNode* childNode = FindChildNode(myCurrentNode, aName);
	if(childNode == nullptr)
	{
		childNode = GetNewNode();
		childNode->myParent = myCurrentNode;

		childNode->mySibling = myCurrentNode->myFirstChild;
		myCurrentNode->myFirstChild = childNode;

		childNode->myName = aName;
		childNode->myColor = aColor;
	}

	myCurrentNode = childNode;
	myCurrentNode->myCurrentStartTime = start.QuadPart;

	QueryPerformanceCounter(&start);
	myProfilerOverhead += (start.QuadPart - startOverheadTime);
}

void GO_APIProfiler::EndProfile(const char* aName)
{
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	long long profileEndTime = start.QuadPart;

	assert(myCurrentNode->myName == aName);
	assert(myCurrentNode->myParent != nullptr);

	myCurrentNode->myHitCount++;

	myCurrentNode->myAccumulator += (profileEndTime - myCurrentNode->myCurrentStartTime);
	//myCurrentNode->myCurrentStartTime = 0;

	myCurrentNode = myCurrentNode->myParent;

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

#endif