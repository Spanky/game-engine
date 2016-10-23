#pragma once

#include "GO_ThreadSafeQueue.h"
#include "GO_ThreadJoiner.h"

class GO_APIProfiler;

namespace GO
{
	class ThreadPool
	{
	public:
		ThreadPool(GO_APIProfiler* aProfiler, unsigned int aNumThreads);
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
		// NOTE: The order of these members matters. The thread joiner must be last as it cannot
		//		 be destroyed before the list of threads. The list of threads cannot be destroyed
		//		 before the queue of work they are using.
		// --------------------------------
		std::atomic_bool myIsDone;
		ThreadSafeQueue<std::packaged_task<int()>> myWorkQueue;
		std::vector<std::thread> myThreads;
		ThreadJoiner myThreadJoiner;
		// --------------------------------

		GO_APIProfiler* myProfiler;
	};
}