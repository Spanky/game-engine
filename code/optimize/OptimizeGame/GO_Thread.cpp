#include "StableHeaders.h"
#include "GO_Thread.h"

namespace GO
{
	Thread::Thread(std::function<void()> aCallback)
		: myThread(aCallback)
	{
	}

	void Thread::join()
	{
		GO_ASSERT(isJoinable(), "Thread is not in a joinable state");
		myThread.join();
	}

	bool Thread::isJoinable()
	{
		return myThread.joinable();
	}
}