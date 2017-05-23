#include "StableHeaders.h"
#include "GO_CalculateCombatDamageSystem.h"

#include "GO_Profiler.h"
#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_ThreadUtils.h"
#include "GO_EventBroker.h"

namespace GO
{
	static float DistanceBetweenEntities(const GO::Entity* aLHS, const GO::Entity* aRHS)
	{
		const sf::Vector2i& lhsPosition = aLHS->getPosition();
		const sf::Vector2i& rhsPosition = aRHS->getPosition();

		return std::sqrt(((lhsPosition.x - rhsPosition.x) * (lhsPosition.x - rhsPosition.x))
			+ ((lhsPosition.y - rhsPosition.y) * (lhsPosition.y - rhsPosition.y)));
	}

	void CalculateCombatDamageSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		GO::World& world = someUpdateParams.myWorld;
		GO::ThreadPool& threadPool = someUpdateParams.myThreadPool;
		GO_APIProfiler& profiler = someUpdateParams.myProfiler;

		PROFILER_SCOPED(&profiler, "CalculateCombatDamageSystem", 0xff5500ff);

		GO::World::EntityList& entityList = world.getEntities();
		const size_t entityListSize = entityList.size();

		ThreadUtils::RunParallel(entityList.begin(), entityList.end(), world, threadPool, profiler, GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK,
			[&](GO::Entity** someEntities, const size_t aStartIndex, const size_t anEndIndex, const size_t aTaskIndex, const unsigned int aTotalTaskCount, GO_APIProfiler& aProfiler)
		{
			PROFILER_SCOPED(&profiler, "CalculateCombatDamageSystem_Threaded", 0x992200ff);
			const GO::World::EntityList& entityList = world.getEntities();
			const size_t entityListSize = entityList.size();

			std::vector<CombatDamageMessage> damageMessages;

			const size_t numTotalIterations = (entityListSize * (entityListSize - 1)) / 2;
			const size_t numIterationsPerTask = (numTotalIterations + aTotalTaskCount - 1) / aTotalTaskCount;

			// An over estimated iterations required by this task has now been calculated. The cut off
			// iteration count will be based on full passes of the inner loop. That is, the inner loop
			// will never do partial iterations. At most, a task will perform one full inner iteration
			// more than a perfectly balanced system

			// We need to loop over all the previous task indices and figure out how many tasks they will
			// be taking. We can then start ours based on this.
			// TODO(scarroll): This can be solved by subtracting the summation of values before our desired
			//					n and solving.

			// https://stackoverflow.com/questions/362059/what-is-the-big-o-of-a-nested-loop-where-number-of-iterations-in-the-inner-loop/362092#362092
			// Calculate n such that summation equals numIterationsPerTask
			// sum = n(n+1) / 2
			// sum * 2 = n * (n+1)
			// sum * 2 = n^2 + n
			// sum * 2 = n^2 + n + (1/2)^2 - (1/2)^2
			// sum * 2 = (n + 0.5)^2 - (0.5)^2
			// sum * 2 + 0.25 = (n + 0.5)^2
			// sqrt(sum * 2 + 0.25) = n + 0.5
			// sqrt(sum * 2 + 0.25) - 0.5 = n

			std::vector<size_t> taskSumCounts(aTotalTaskCount);

			using indexPair = std::pair<size_t, size_t>;

			std::vector<indexPair> taskIndices(aTotalTaskCount);
			for (unsigned int i = 0; i < aTotalTaskCount; i++)
			{
				size_t sumRequired = (i > 0) ? (taskSumCounts[i - 1] + numIterationsPerTask) : numIterationsPerTask;
				taskSumCounts[i] = sumRequired;

				indexPair indices;
				indices.first = (i > 0) ? taskIndices[i - 1].second : 0;
				indices.second = std::round(sqrt(sumRequired * 2 + 0.25) - 0.5);

				taskIndices[i] = indices;
			}

			for (unsigned int i = 0; i < aTotalTaskCount; i++)
			{
				indexPair& indices = taskIndices[i];

				std::swap(indices.first, indices.second);
				indices.first = entityListSize - indices.first - 1;
				indices.second = entityListSize - indices.second - 1;
			}

			for (size_t outerIndex = taskIndices[aTaskIndex].first; outerIndex < taskIndices[aTaskIndex].second; outerIndex++)
			{
				GO::Entity* outerEntity = someEntities[outerIndex];
				GO_ASSERT(outerEntity, "Entity list contains a nullptr");

				for (size_t innerIndex = outerIndex + 1; innerIndex < entityListSize; innerIndex++)
				{
					GO::Entity* innerEntity = entityList[innerIndex];
					GO_ASSERT(innerEntity, "Entity list contains a nullptr");

					const float distance = DistanceBetweenEntities(outerEntity, innerEntity);
					if (distance < 100.0f)
					{
						{
							CombatDamageMessage msg;
							msg.myDealerEntity = outerEntity;
							msg.myReceiverEntity = innerEntity;
							msg.myDamageDealt = 1.0f;
							damageMessages.push_back(msg);
						}

						{
							CombatDamageMessage msg;
							msg.myDealerEntity = innerEntity;
							msg.myReceiverEntity = outerEntity;
							msg.myDamageDealt = 1.0f;
							damageMessages.push_back(msg);
						}
					}
				}
			}

			// TODO(scarroll): Perform the gather in parallel with other tasks
			GO::EventBroker::QueueDamage(damageMessages, profiler);
		});
	}
}