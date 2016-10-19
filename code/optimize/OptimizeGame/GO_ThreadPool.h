#pragma once

#include "GO_ThreadSafeQueue.h"
#include "GO_ThreadJoiner.h"

class GO_APIProfiler;

namespace GO
{
	class ThreadPool
	{
	public:
		ThreadPool(GO_APIProfiler* aProfiler);
		~ThreadPool();

		template<typename FunctionType>
		std::future<typename std::result_of<FunctionType()>::type> Submit(FunctionType aFunction)
		{
			typedef typename std::result_of<FunctionType()>::type result_type;

			std::packaged_task<result_type()> task(std::move(aFunction));
			std::future<result_type> res(task.get_future());

			myWorkQueue.Push(std::move(task));

			return res;
		}

	private:
		void WorkerThread();

	private:
		std::atomic_bool myIsDone;
		ThreadSafeQueue<std::packaged_task<int()>> myWorkQueue;
		std::vector<std::thread> myThreads;
		ThreadJoiner myThreadJoiner;

		GO_APIProfiler* myProfiler;
	};
}