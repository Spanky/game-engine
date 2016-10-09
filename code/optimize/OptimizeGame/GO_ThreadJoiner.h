#pragma once

namespace GO
{
	class ThreadJoiner
	{
	public:
		explicit ThreadJoiner(std::vector<std::thread>& someThreads)
			: myThreads(someThreads)
		{
		}

		~ThreadJoiner()
		{
			for (size_t i = 0; i < myThreads.size(); i++)
			{
				if (myThreads[i].joinable())
				{
					myThreads[i].join();
				}
			}
		}

	private:
		std::vector<std::thread>& myThreads;
	};
}