#include "StableHeaders.h"
#include "GO_UniformThreadGroup.h"

#include "GO_Thread.h"

namespace GO
{
	UniformThreadGroup::UniformThreadGroup(const int aNumThreads, std::function<void()>* aCallbackList)
		: myThreads(nullptr)
		, myNumThreads(aNumThreads)
	{
		GO_ASSERT(myNumThreads > 0, "Must create at least 1 thread");

		myThreads = new Thread*[myNumThreads];
		for (int i = 0; i < myNumThreads; i++)
		{
			myThreads[i] = new Thread(aCallbackList[i]);
		}
	}

	UniformThreadGroup::~UniformThreadGroup()
	{
		if (isJoinable())
		{
			joinThreads();
		}

		for (int i = myNumThreads - 1; i >= 0; i--)
		{
			delete myThreads[i];
			myThreads[i] = nullptr;
		}

		delete[] myThreads;
		myThreads = nullptr;
	}

	void UniformThreadGroup::join()
	{
		if (isJoinable())
		{
			joinThreads();
		}
	}

	void UniformThreadGroup::joinThreads()
	{
		for (int i = 0; i < myNumThreads; i++)
		{
			myThreads[i]->join();
		}
	}

	bool UniformThreadGroup::isJoinable()
	{
		// TODO: Add some debug code that will check all threads for joinability
		return myThreads[0]->isJoinable();
	}
}