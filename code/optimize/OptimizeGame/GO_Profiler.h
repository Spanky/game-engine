#pragma once

#include "GO_AssertMutex.h"

#define PROFILER_ENABLED 1

#if PROFILER_ENABLED

unsigned char GetCurrentThreadIndex();

static constexpr unsigned char MaxThreadCount = UCHAR_MAX;

struct CallGraphNode
{
	// 0
	const char* myName;

	// 8
	long long myStartTime;
	long long myEndTime;

	//24
	unsigned int myColor;

	// 32
	CallGraphNode* myLastChild;
	CallGraphNode* myPrevSibling;
	CallGraphNode* myParent;

	unsigned char myPadding[8];

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
	using FrameCallGraphRoots = std::array < CallGraphNode*, MaxThreadCount >;

	GO_APIProfiler();
	GO_APIProfiler(const GO_APIProfiler& aRHS) = delete;
	GO_APIProfiler& operator=(const GO_APIProfiler& aRHS) = delete;

	void BeginFrame();
	void EndFrame();

	void PauseCollection();
	void ResumeCollection();

	// TODO(scarroll): These are view related and do not belong on the actual profiler
	void ViewPrevFrame();
	void ViewNextFrame();

	void BeginProfile(const char* aName, unsigned int aColor);
	void EndProfile(const char* aName);

	void PushThreadEvent(unsigned char aThreadTag);

	const std::vector<ProfilerThreadEvent>& GetPreviousFrameThreadEvents()
	{
		return myPreviousThreadEvents;
	}

	long long GetPreviousFrameStartTime() const
	{
		return myPreviousFrameStartTime;
	}

	long long GetPreviousFrameEndTime() const
	{
		return myPreviousFrameEndTime;
	}

	long long GetPreviousFrameDurationTicks() const
	{
		return myPreviousFrameEndTime - myPreviousFrameStartTime;
	}

	static float TicksToMilliseconds(long long aTickCount);

	// TODO: These methods should not expose internals as to how they are implemented
	const FrameCallGraphRoots& GetPreviousFrameCallGraphRoots() const;

private:
	void ReleaseNodeHierarchy(CallGraphNode* aCallGraphNode);

	void StoreFrameInHistory();

	GO::AssertMutex myProfilerEventMutex;


private:
	static double ourFrequency;

	long long myPreviousFrameStartTime;
	long long myPreviousFrameEndTime;

	long long myCurrentFrameStartTime;
	long long myCurrentFrameEndTime;

	long long myProfilerOverhead;

	std::vector<ProfilerThreadEvent> myThreadEvents;
	std::vector<ProfilerThreadEvent> myPreviousThreadEvents;

	bool myIsCollectionActive;
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