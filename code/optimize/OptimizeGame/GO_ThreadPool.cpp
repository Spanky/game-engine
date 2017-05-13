#include "StableHeaders.h"
#include "GO_ThreadPool.h"
#include "GO_Profiler.h"
#include "GO_ProfilerTags.h"

#include <memory>

namespace GO
{
	ThreadPool::ThreadPool(GO_APIProfiler* aProfiler, unsigned int aNumThreads)
		: myIsDone(false)
		, myThreadJoiner(myThreads)
		, myProfiler(aProfiler)
	{
		unsigned int threadCount = aNumThreads ? aNumThreads : std::thread::hardware_concurrency();

		for (unsigned int i = 0; i < threadCount; i++)
		{
			myThreads.push_back(std::thread(&ThreadPool::WorkerThread, this));
		}
	}

	ThreadPool::~ThreadPool()
	{
		// Notify all threads that we want them to shutdown
		myIsDone = true;
		myWakeWorkerThreadConditional.notify_all();

		// NOTE: The destructor of the thread joiner prevents this method from exiting until all threads are terminated
	}

	void ThreadPool::WorkerThread()
	{
		myProfiler->PushThreadEvent(GO_ProfilerTags::THREAD_TAG_OVERHEAD);

		while(!myIsDone)
		{
			std::packaged_task<int()> task;
			if (myWorkQueue.TryPop(task))
			{
				// TODO: We should be grabbing the current threads tag and storing that
				//		 internally when we actually submit a task into the system. At
				//		 that point we can assert that we have a valid thread tag so that
				//		 no one can schedule a task without a proper tag
				task();

				// TODO: We may get context swapped out here since all of our futures are complete
				//		 and the main thread starts running now. We need to find a way to push this
				//		 event on without getting swapped out (we will show longer than we actually
				//		 were)
				myProfiler->PushThreadEvent(GO_ProfilerTags::THREAD_TAG_OVERHEAD);
			}
			else
			{
				myProfiler->PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);

				std::unique_lock<std::mutex> lock(myHasWorkMutex);
				myWakeWorkerThreadConditional.wait(lock, [this] { return myIsDone || !myWorkQueue.IsEmpty(); });

				// TODO: We may get context swapped out here since all of our futures are complete
				//		 and the main thread starts running now. We need to find a way to push this
				//		 event on without getting swapped out (we will show longer than we actually
				//		 were)
				myProfiler->PushThreadEvent(GO_ProfilerTags::THREAD_TAG_OVERHEAD);
			}
		}

		// Complete remaining tasks that are still in the queue
		std::packaged_task<int()> task;
		while (myWorkQueue.TryPop(task))
		{
			task();
		}
	}
}