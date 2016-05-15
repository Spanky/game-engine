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

		void Empty();

	private:
		Node* GetTail()
		{
			std::lock_guard<std::mutex> tailLock(myTailMutex);
			return tail;
		}

		std::unique_ptr<Node> PopHead()
		{
			std::unique_ptr<Node> 
		}

	private:
		std::mutex myHeadMutex;
		std::unique_ptr<Node> myHead;

		std::mutex myTailMutex;
		Node* myTail;

		std::condition_variable myDataConditional;
	};

	template<typename T>
	void ThreadSafeQueue::Push(T aNewValue)
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


		std::shared_ptr<T> TryPop()
		{
			if(!myHead)
			{
				return std::shared_ptr<T>();
			}

			if (myHead.get() == myTail)
			{
				myTail = nullptr;
			}

			const std::shared_ptr<T> result(std::make_shared<T>(std::move(myHead->myData)));
			myHead = std::move(myHead->myNext);
			return result;
		}

		void Push(T aNewValue)
		{
			std::unique_ptr<Node> p(new Node(std::move(aNewValue)));
			Node* const newTail = p.get();
			
			if(myTail)
			{
				myTail->myNext = std::move(p);
			}
			else
			{
				myHead = std::move(p);
			}

			myTail = newTail;
		}
	};
}