#pragma once

namespace GO
{
	template<typename T>
	class ThreadSafeQueue
	{
	private:
		struct Node
		{
			std::shared_ptr<T> myData;
			std::unique_ptr<Node> myNext;
		};

	public:
		ThreadSafeQueue()
			: myHead(new Node())
			, myTail(myHead.get())
		{
		}

		ThreadSafeQueue(const ThreadSafeQueue& aRHS) = delete;
		ThreadSafeQueue& operator=(const ThreadSafeQueue& aRHS) = delete;

		std::shared_ptr<T> TryPop();
		bool TryPop(T& aValue);

		std::shared_ptr<T> WaitAndPop();
		void WaitAndPop(T& aValue);

		void Push(T aNewValue);

		bool IsEmpty() const;

	private:
		Node* GetTail()
		{
			std::lock_guard<std::mutex> tailLock(myTailMutex);
			return myTail;
		}

		std::unique_ptr<Node> PopHeadUnlocked()
		{
			std::unique_ptr<Node> oldHead = std::move(myHead);
			myHead = std::move(oldHead->myNext);
			return oldHead;
		}

		std::unique_lock<std::mutex> WaitForData()
		{
			std::unique_lock<std::mutex> headLock(myHeadMutex);
			myDataConditional.wait(headLock, [&] { return myHead.get() != GetTail(); });
			return std::move(headLock);
		}

		std::unique_ptr<Node> WaitPopHead()
		{
			std::unique_lock<std::mutex> headLock(WaitForData());
			return PopHeadUnlocked();
		}

		std::unique_ptr<Node> WaitPopHead(T& aValue)
		{
			std::unique_lock<std::mutex> headLock(WaitForData());
			aValue = std::move(myHead->myData);
			return PopHeadUnlocked();
		}

		std::unique_ptr<Node> TryPopHead()
		{
			std::lock_guard<std::mutex> headLock(myHeadMutex);
			if (myHead.get() == GetTail())
			{
				return std::unique_ptr<Node>();
			}

			return PopHeadUnlocked();
		}

		std::unique_ptr<Node> TryPopHead(T& aValue)
		{
			std::lock_guard<std::mutex> headLock(myHeadMutex);
			if (myHead.get() == GetTail())
			{
				return std::unique_ptr<Node>();
			}

			aValue = std::move(*myHead->myData.get());
			return PopHeadUnlocked();
		}

	private:
		std::mutex myHeadMutex;
		std::unique_ptr<Node> myHead;

		std::mutex myTailMutex;
		Node* myTail;

		std::condition_variable myDataConditional;
	};

	template<typename T>
	void ThreadSafeQueue<T>::Push(T aNewValue)
	{
		std::shared_ptr<T> newData(std::make_shared<T>(std::move(aNewValue)));

		std::unique_ptr<Node> node(new Node());

		{
			std::lock_guard<std::mutex> tailLock(myTailMutex);
			myTail->myData = newData;
			myTail->myNext = std::move(node);
			myTail = myTail->myNext.get();
		}

		myDataConditional.notify_one();
	}

	template<typename T>
	std::shared_ptr<T> ThreadSafeQueue<T>::WaitAndPop()
	{
		const std::unique_ptr<Node> oldHead = WaitPopHead();
		return oldHead->myData;
	}

	template<typename T>
	void ThreadSafeQueue<T>::WaitAndPop(T& aValue)
	{
		const std::unique_ptr<Node> oldHead = WaitPopHead(aValue);
	}

	template<typename T>
	std::shared_ptr<T> ThreadSafeQueue<T>::TryPop()
	{
		const std::unique_ptr<Node> oldHead = TryPopHead();
		return oldHead ? oldHead->myData : std::shared_ptr<T>();
	}

	template<typename T>
	bool ThreadSafeQueue<T>::TryPop(T& aValue)
	{
		const std::unique_ptr<Node> oldHead = TryPopHead(aValue);
		return oldHead.get() != nullptr;
	}

	template<typename T>
	bool ThreadSafeQueue<T>::IsEmpty() const
	{
		std::lock_guard<std::mutex> headLock(myHeadMutex);
		return myHead.get() == GetTail();
	}
}