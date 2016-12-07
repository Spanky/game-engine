#pragma once

namespace GO
{
	// Modeled after listing 6.2 from C++ Concurrency in Action
	template<typename T>
	class LockingThreadSafeQueue
	{
	public:
		LockingThreadSafeQueue()
		{
		}

		void Push(T anItemToPush)
		{
			// TODO: Investigate if we should pass a const-ref here or just leave it as a copy
			std::lock_guard<std::mutex> lock(myMutex);
			myDataQueue.push(std::move(anItemToPush));
			myNotificationCondition.notify_one();
		}

		void WaitAndPop(T& aPoppedValue)
		{
			std::unique_lock<std::mutex> lock(myMutex);
			myNotificationCondition.wait(lock, [this] { return !myDataQueue.empty(); });

			aPoppedValue = std::move(myDataQueue.front());
			myDataQueue.pop();
		}

		bool TryPop(T& aPoppedValue)
		{
			std::lock_guard<std::mutex> lock(myMutex);
			if(myDataQueue.empty())
			{
				return false;
			}

			aPoppedValue = std::move(myDataQueue.front());
			myDataQueue.pop();

			return true;
		}

		bool IsEmpty() const
		{
			std::lock_guard<std::mutex> lock(myMutex);
			return myDataQueue.empty();
		}

	private:
		mutable std::mutex myMutex;
		std::queue<T> myDataQueue;
		std::condition_variable myNotificationCondition;
	};
}