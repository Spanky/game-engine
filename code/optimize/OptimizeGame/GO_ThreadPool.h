#pragma once

#include "GO_ThreadSafeQueue.h"

namespace GO
{
	class ThreadJoiner
	{
	public:
		explicit ThreadJoiner(std::vector<std::thread>& someThreads)
			: myThreads(someThreads)
		{
		}

		~ThreadJoiner()
		{
			for (size_t i = 0; i < myThreads.size(); i++)
			{
				if (myThreads[i].joinable())
				{
					myThreads[i].join();
				}
			}
		}

	private:
		std::vector<std::thread>& myThreads;
	};


	class ThreadPool
	{
	public:
		ThreadPool();
		~ThreadPool();

		template<typename FunctionType>
		void Submit(FunctionType aFunction)
		{
			myWorkQueue.Push(std::function<void()>(aFunction));
		}

	private:
		void WorkerThread();

	private:
		std::atomic_bool myIsDone;
		ThreadSafeQueue<std::function<void()>> myWorkQueue;
		std::vector<std::thread> myThreads;
		ThreadJoiner myThreadJoiner;
	};
}