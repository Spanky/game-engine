#include "StableHeaders.h"
#include "GO_AssertMutex.h"

namespace GO
{
	AssertMutex::AssertMutex()
		: myNumRecursiveLocks(0)
	{
		myDefaultThreadIdHash = std::hash<std::thread::id>()(std::thread::id());
		myLockedThreadIdHash = myDefaultThreadIdHash;
	}


	bool AssertMutex::TryLockForThread()
	{
		std::thread::id currentThreadId = std::this_thread::get_id();
		size_t currentThreadIdHash = std::hash<std::thread::id>()(currentThreadId);

		// TODO: If the lock isn't available for this thread, don't even try to lock it
		bool lockObtained = myMutex.try_lock();

		// We couldn't obtain the lock so we'll assert and dump out who currently owns the lock
		if (lockObtained)
		{
			// TODO: Wrap these 2 lines in debug-only validation code
			size_t ownerThreadIdHash = std::atomic_load(&myLockedThreadIdHash);
			GO_ASSERT(ownerThreadIdHash == myDefaultThreadIdHash || ownerThreadIdHash == currentThreadIdHash, "Mutex was owned by another thread"); // Validate that our 'current thread' ID is correct for who owns the lock

			std::atomic_store(&myLockedThreadIdHash, currentThreadIdHash);
			std::atomic_fetch_add(&myNumRecursiveLocks, 1);
		}
		else
		{
			size_t lockedThreadIdHash = std::atomic_load(&myLockedThreadIdHash);

			std::stringstream ss;
			ss << lockedThreadIdHash << std::endl;
			GO_ASSERT(false && "Thread Contention disallowed on AssertMutex", ss.str().c_str());
			return false;
		}
		return lockObtained;
	}

	bool AssertMutex::UnlockForThread()
	{
		std::thread::id currentThreadId = std::this_thread::get_id();
		size_t currentThreadIdHash = std::hash<std::thread::id>()(currentThreadId);

		size_t lockOwnedByThreadIdHash = std::atomic_load(&myLockedThreadIdHash);

		bool lockIsOwnedByThisThread = currentThreadIdHash == lockOwnedByThreadIdHash;

		if (lockIsOwnedByThisThread)
		{
			myMutex.unlock();
			size_t numPreviousLocks = std::atomic_fetch_sub(&myNumRecursiveLocks, 1);

			if (numPreviousLocks == 1)
			{
				assert(std::atomic_load(&myLockedThreadIdHash) == currentThreadIdHash);
				std::atomic_store(&myLockedThreadIdHash, myDefaultThreadIdHash);
			}
		}

		return true;
	}




	AssertMutexLock::AssertMutexLock(AssertMutex& aMutex)
		: myMutex(aMutex)
	{
		Lock();
	}

	AssertMutexLock::~AssertMutexLock()
	{
		Unlock();
	}

	void AssertMutexLock::Lock()
	{
		myMutex.TryLockForThread();
	}

	void AssertMutexLock::Unlock()
	{
		// TODO: How do we know that we have the lock here?
		myMutex.UnlockForThread();
	}
}