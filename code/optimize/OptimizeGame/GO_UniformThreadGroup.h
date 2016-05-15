#pragma once

namespace GO
{
	class Thread;
}

namespace GO
{
	class UniformThreadGroup
	{
	public:
		UniformThreadGroup(const int aNumThreads, std::function<void()>* aCallbackList);
		~UniformThreadGroup();

		void join();
		bool isJoinable();

	private:
		void joinThreads();

	private:
		Thread** myThreads;
		int myNumThreads;
	};
}