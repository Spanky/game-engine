#include "StableHeaders.h"
#include "GO_ThreadPool.h"

namespace GO
{
	ThreadPool::ThreadPool()
		: myIsDone(false)
		, myThreadJoiner(myThreads)
	{
		unsigned int threadCount = std::thread::hardware_concurrency();

		for (unsigned int i = 0; i < threadCount; i++)
		{
			myThreads.push_back(std::thread(&ThreadPool::WorkerThread, this));
		}
	}

	ThreadPool::~ThreadPool()
	{
		myIsDone = true;
	}

	void ThreadPool::WorkerThread()
	{
		while(!myIsDone)
		{
			std::function<void()> task;
			if (myWorkQueue.TryPop(task))
			{
				task();
			}
			else
			{
				std::this_thread::yield();
			}
		}

		// Complete remaining tasks that are still in the queue
		std::function<void()> task;
		while (myWorkQueue.TryPop(task))
		{
			task();
		}
	}
}