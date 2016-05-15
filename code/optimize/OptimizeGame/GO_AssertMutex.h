#pragma once


namespace GO
{
	class AssertMutex
	{
	public:
		AssertMutex();

		bool TryLockForThread();
		bool UnlockForThread();

	private:
		std::recursive_mutex myMutex;
		std::atomic_size_t myNumRecursiveLocks;

		std::atomic_size_t myLockedThreadIdHash;
		size_t myDefaultThreadIdHash;
	};



	class AssertMutexLock
	{
	public:
		AssertMutexLock(AssertMutex& aMutex);
		~AssertMutexLock();

	private:
		void Lock();
		void Unlock();

	private:
		AssertMutex& myMutex;
	};
}