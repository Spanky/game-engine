#include "StableHeaders.h"
#include "GO_Profiler.h"

#if PROFILER_ENABLED

struct ProfilerNode
{
	// 0
	const char* myName;

	// 8
	ProfilerNode* myFirstChild;
	ProfilerNode* mySibling;
	ProfilerNode* myParent;

	// 32
	long long myAccumulator;
	long long myHitCount;

	// 48
	long long myCurrentStartTime;

	long long myPadding;

	// 64
};

// TODO: Move this to a common 'standards' file
static const int CacheLineSizeInBytes = 64;

static_assert(sizeof(ProfilerNode) <= CacheLineSizeInBytes, "ProfilerNode needs to fit in a single cache line");
static_assert((CacheLineSizeInBytes % sizeof(ProfilerNode)) == 0, "ProfilerNode needs to fit evenly in a cache line");


ProfilerNode locRootNode;

double GO_APIProfiler::ourFrequency = 0;
long long GO_APIProfiler::ourReportInterval = 0;

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

	void ResetRootNode()
	{
		ResetNode(locRootNode);
		locRootNode.myName = "Root";
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
	: myLastFlushedOutputTime(0)
	, myCurrentNode(nullptr)
{
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	myStart = start.QuadPart;

	ResetRootNode();
}


void GO_APIProfiler::BeginFrame()
{
	locRootNode.myCurrentStartTime = locGetCurrentProfilerTime();
	myProfilerOverhead = 0;

	myCurrentNode = &locRootNode;
}

void GO_APIProfiler::EndFrame()
{
	assert(myCurrentNode == &locRootNode);
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

	locRootNode.myAccumulator += (frameEndTime - locRootNode.myCurrentStartTime);
	locRootNode.myHitCount++;

	if (locRootNode.myAccumulator > ourReportInterval)
	{
		Flush(frameEndTime);
		ReleaseAllProfilerNodes();
	}
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

void GO_APIProfiler::ReleaseAllProfilerNodes()
{
	// NOTE: We do this manually since we do not want to call 'delete' on our root node.
	// TODO: Add a parameter to determine if we need to delete the node or not?
	ProfilerNode* childNode = locRootNode.myFirstChild;
	while (childNode)
	{
		ProfilerNode* nextChild = childNode->mySibling;
		ReleaseNodeHierarchy(childNode);

		childNode = nextChild;
	}

	ResetRootNode();
}

ProfilerNode* GO_APIProfiler::GetNewNode()
{
	ProfilerNode* profilerNode = new ProfilerNode();
	ResetNode(*profilerNode);

	return profilerNode;
}

void GO_APIProfiler::BeginProfile(const char* aName)
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

void print_with_indent(int indent, char * string)
{
	printf("%*s %s", indent, "", string);
}

void GO_APIProfiler::PrintProfilerHierarchy(ProfilerNode* aRootNode, double anInterval, int aLevel)
{
	double measured = aRootNode->myAccumulator * ourFrequency;

	char buffer[256];
	sprintf(buffer,
	"%*s TID 0x%x time spent in \"%s\": %0.f/%0.f ms %0.f %.1f%% %I64d\n",
		aLevel * 2,
		"",
		GetCurrentThreadId(),
		aRootNode->myName,

		measured * 1000,
		anInterval * 1000,
		(measured * 1000 / aRootNode->myHitCount) * 1000,
		100.f * measured / anInterval,
		aRootNode->myHitCount);
	OutputDebugString(buffer);


	ProfilerNode* childNode = aRootNode->myFirstChild;
	while (childNode)
	{
		ProfilerNode* nextChild = childNode->mySibling;
		PrintProfilerHierarchy(childNode, anInterval, aLevel + 1);

		childNode = nextChild;
	}
}

void GO_APIProfiler::Flush(long long anEnd)
{
	if(ourReportInterval == 0)
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		ourFrequency = 1.0 / freq.QuadPart;
		MemoryBarrier();
		ourReportInterval = freq.QuadPart;
	}

	if(myLastFlushedOutputTime == 0)
	{
		myLastFlushedOutputTime = myStart;
		return;
	}

	double interval = (anEnd - myLastFlushedOutputTime) * ourFrequency;
	myLastFlushedOutputTime = anEnd;

	char buffer[200];
	sprintf(buffer, "Overhead: %0.f\n", myProfilerOverhead * ourFrequency * 1000);
	OutputDebugString(buffer);

	PrintProfilerHierarchy(myCurrentNode, interval, 0);
}

#endif