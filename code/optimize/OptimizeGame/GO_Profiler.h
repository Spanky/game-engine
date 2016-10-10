#pragma once

#define ENABLE_API_PROFILER 1

#if ENABLE_API_PROFILER

// Based on the work of Jeff Phreshing here
//		http://preshing.com/20111203/a-c-profiling-module-for-multithreaded-apis/
class GO_APIProfiler
{
public:
	struct ThreadInfo
	{
		long long myLastReportTime;
		long long myAccumulator;
		long long myHitCount;
		const char* myName;
	};

private:
	long long myStart;
	ThreadInfo* myThreadInfo;

	static double ourFrequency;
	static long long ourReportInterval;

	void Flush(long long anEnd);

public:
	GO_APIProfiler(ThreadInfo* aThreadInfo)
	{
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);
		myThreadInfo = aThreadInfo;
		myStart = start.QuadPart;
	}

	~GO_APIProfiler()
	{
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		myThreadInfo->myAccumulator += (end.QuadPart - myStart);
		myThreadInfo->myHitCount++;
		if(end.QuadPart - myThreadInfo->myLastReportTime > ourReportInterval)
		{
			Flush(end.QuadPart);
		}
	}
};

#define TOKENPASTE2(x, y) x ## y
#define TOKENPASTE(x, y) TOKENPASTE2(x, y)

#define DECLARE_API_PROFILER(name)		extern GO_APIProfiler::ThreadInfo __GO_APIProfiler_##name;
#define DEFINE_API_PROFILER(name)		GO_APIProfiler::ThreadInfo __GO_APIProfiler_##name = { 0, 0, 0, #name };

#define API_PROFILER(name)				\
	GO_APIProfiler TOKENPASTE(__GO_APIProfiler_##name, __LINE__)(&__GO_APIProfiler_##name)


#else		// #if ENABLE_API_PROFILER

#define DECLARE_API_PROFILER(name)
#define DEFINE_API_PROFILER(name)
#define API_PROFILER(name);


#endif	// ENABLE_API_PROFILER