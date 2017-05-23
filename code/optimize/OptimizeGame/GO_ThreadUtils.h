#pragma once

#include "GO_Profiler.h"
#include "GO_ThreadPool.h"

namespace GO
{
	namespace ThreadUtils
	{
		template<typename Iterator, typename Callback, typename Type = typename std::iterator_traits<Iterator>::value_type>
		void RunParallel(Iterator anIterBegin, Iterator anIterEnd, GO::World &aWorld, GO::ThreadPool &aThreadPool, GO_APIProfiler &aProfiler, const unsigned char aThreadTag, Callback aCallback)
		{
			if (anIterBegin == anIterEnd)
			{
				return;
			}

			// TODO(scarroll): Get the number of tasks available from the thread pool
			const unsigned int threadCount = std::thread::hardware_concurrency();

			size_t numElements = anIterEnd - anIterBegin;

			// We have at least one job for every thread. Otherwise we'll just do the processing ourselves
			if (numElements >= threadCount)
			{
				const size_t NumJobsPerThread = (numElements + threadCount - 1) / threadCount;

				std::atomic_size_t counter = 0;
				{
					std::vector<std::future<int>> futures(threadCount - 1);

					// Run the parallel batches
					for (size_t i = 0; i < threadCount - 1; ++i)
					{
						const size_t startIndex = i * NumJobsPerThread;
						const size_t endIndex = (i + 1) * NumJobsPerThread;

						std::future<int> result = aThreadPool.Submit([&counter, &aProfiler, startIndex, endIndex, anIterBegin, aThreadTag, aCallback, i, threadCount]() -> int
						{
							PROFILER_SCOPED(&aProfiler, "Run Parallel", 0x00A2E8FF);
							// TODO: The thread tag should be automatically gathered from the same tag as the 'submit' event
							aProfiler.PushThreadEvent(aThreadTag);
							aCallback(anIterBegin.operator->(), startIndex, endIndex, i, threadCount, aProfiler);

							const size_t numProcessed = endIndex - startIndex;

							// TODO(scarroll): Remove the counter for batch processing once the system is complete
							return int(counter.fetch_add(numProcessed));
						});

						futures[i] = std::move(result);
					}


					// Run the local batch
					{
						const size_t startIndex = (threadCount - 1) * NumJobsPerThread;
						const size_t numProcessed = numElements - startIndex;
						aCallback(anIterBegin.operator->(), startIndex, numElements, threadCount - 1, threadCount, aProfiler);
						counter.fetch_add(numProcessed);
					}

					std::for_each(futures.begin(), futures.end(), [](std::future<int>& aFuture)
					{
						aFuture.get();
					});
				}


				size_t resultCounter = counter.load();
				GO_ASSERT(resultCounter == numElements, "Number of jobs processed was not the same as the number of jobs submitted");
			}
			else
			{
				// Run everything locally since it's a small batch anyways
				aCallback(anIterBegin.operator->(), 0, numElements, 0, 1, aProfiler);
			}
		}
	}
}