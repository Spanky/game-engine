#pragma once

#define PROFILER_ENABLED 1

#if PROFILER_ENABLED

unsigned char GetCurrentThreadIndex();

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
	unsigned int myColor;

	// 60
	unsigned int myPadding;
	
	// 64
};


struct ProfilerThreadEvent
{
	long long myStartTime;

	unsigned int myThreadIndex;
	unsigned char myThreadTag;
};

// Based on the work of Jeff Phreshing here
//		http://preshing.com/20111203/a-c-profiling-module-for-multithreaded-apis/
class GO_APIProfiler
{
public:
	GO_APIProfiler();
	GO_APIProfiler(const GO_APIProfiler& aRHS) = delete;
	GO_APIProfiler& operator=(const GO_APIProfiler& aRHS) = delete;

	void BeginFrame();
	void EndFrame();

	void BeginProfile(const char* aName, unsigned int aColor);
	void EndProfile(const char* aName);

	void PushThreadEvent(unsigned char aThreadTag);

	// TODO: These methods should not expose internals as to how they are implemented
	ProfilerNode* GetPreviousFrameRootNode()
	{
		return myPreviousFrameRootNode;
	}

	const std::vector<ProfilerThreadEvent>& GetPreviousFrameThreadEvents()
	{
		return myPreviousThreadEvents;
	}

	long long GetPreviousFrameStartTime() const
	{
		return myPreviousFrameStartTime;
	}

	static float TicksToMilliseconds(long long aTickCount);

private:
	ProfilerNode* GetNewNode();
	void ReleaseNodeHierarchy(ProfilerNode* aProfilerNode);

	void StoreFrameInHistory();


private:
	static double ourFrequency;

	long long myStart;
	long long myPreviousFrameStartTime;

	long long myCurrentFrameStartTime;

	long long myProfilerOverhead;
	ProfilerNode* myCurrentNode;
	ProfilerNode* myPreviousFrameRootNode;

	std::vector<ProfilerThreadEvent> myThreadEvents;
	std::vector<ProfilerThreadEvent> myPreviousThreadEvents;
};


class GO_APIProfileInstance
{
public:
	GO_APIProfileInstance(GO_APIProfiler* aProfiler, const char* aName, unsigned int aColor)
		: myProfiler(aProfiler)
		, myName(aName)
	{
		myProfiler->BeginProfile(myName, aColor);
	}

	~GO_APIProfileInstance()
	{
		myProfiler->EndProfile(myName);
	}

private:
	GO_APIProfiler* myProfiler;
	const char* myName;
};



#define TOKENPASTE2(x, y) x ## y
#define TOKENPASTE(x, y) TOKENPASTE2(x, y)


#define PROFILER_BEGIN_FRAME(profiler)		(profiler).BeginFrame();
#define PROFILER_END_FRAME(profiler)		(profiler).EndFrame();

#define PROFILER_SCOPED(profiler, name, color)		GO_APIProfileInstance TOKENPASTE(__GO_APIProfiler_, __LINE__)(profiler, name, color);

#else		// #if PROFILER_ENABLED

#define PROFILER_BEGIN_FRAME(profiler)
#define PROFILER_END_FRAME(profiler)

#define PROFILER_SCOPED(profiler, name, color)


#endif	// PROFILER_ENABLED