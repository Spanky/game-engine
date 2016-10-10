#include "StableHeaders.h"
#include "GO_Profiler.h"

#if ENABLE_API_PROFILER

double GO_APIProfiler::ourFrequency = 0;
long long GO_APIProfiler::ourReportInterval = 0;

void GO_APIProfiler::Flush(long long anEnd)
{
	if(ourReportInterval == 0)
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		ourFrequency = 1.0 / freq.QuadPart;
		ourReportInterval = freq.QuadPart;
		MemoryBarrier();
	}

	if(myThreadInfo->myLastReportTime == 0)
	{
		myThreadInfo->myLastReportTime = myStart;
		return;
	}

	double interval = (anEnd - myThreadInfo->myLastReportTime) * ourFrequency;
	double measured = myThreadInfo->myAccumulator * ourFrequency;

	printf("TID 0x%x time spent in \"%s\": %.0f/%0.f ms %.1f%% %I64dx\n",
		GetCurrentThreadId(),
		myThreadInfo->myName,
		measured * 1000,
		interval * 1000,
		100.f * measured / interval,
		myThreadInfo->myHitCount);

	myThreadInfo->myLastReportTime = anEnd;
	myThreadInfo->myAccumulator = 0;
	myThreadInfo->myHitCount = 0;
}

#endif