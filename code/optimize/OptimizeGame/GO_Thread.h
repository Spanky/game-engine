#pragma once

namespace GO
{
	class Thread
	{
	public:
		Thread(std::function<void()> aCallback);

		bool isJoinable();
		void join();

	private:
		std::thread myThread;
	};
}