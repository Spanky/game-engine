#pragma once

#define PROFILER_ENABLED 1

#if PROFILER_ENABLED

struct ProfilerNode;

// Based on the work of Jeff Phreshing here
//		http://preshing.com/20111203/a-c-profiling-module-for-multithreaded-apis/
class GO_APIProfiler
{
public:
	GO_APIProfiler();

	void BeginFrame();
	void EndFrame();

	void BeginProfile(const char* aName);
	void EndProfile(const char* aName);

private:
	ProfilerNode* GetNewNode();
	
	void ReleaseAllProfilerNodes();
	void ReleaseNodeHierarchy(ProfilerNode* aProfilerNode);

	void Flush(long long anEnd);
	void PrintProfilerHierarchy(ProfilerNode* aRootNode, double anInterval, int aLevel);


private:
	static double ourFrequency;
	static long long ourReportInterval;

	long long myStart;
	long long myLastFlushedOutputTime;
	long long myProfilerOverhead;
	ProfilerNode* myCurrentNode;
};


class GO_APIProfileInstance
{
public:
	GO_APIProfileInstance(GO_APIProfiler* aProfiler, const char* aName)
		: myProfiler(aProfiler)
		, myName(aName)
	{
		myProfiler->BeginProfile(myName);
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

#define PROFILER_SCOPED(profiler, name)		GO_APIProfileInstance TOKENPASTE(__GO_APIProfiler_, __LINE__)(profiler, name);

#else		// #if PROFILER_ENABLED

#define PROFILER_BEGIN_FRAME(profiler)
#define PROFILER_END_FRAME(profiler)

#define PROFILER_SCOPED(profiler, name)


#endif	// PROFILER_ENABLED