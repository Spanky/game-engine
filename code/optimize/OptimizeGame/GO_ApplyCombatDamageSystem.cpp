#include "StableHeaders.h"
#include "GO_ApplyCombatDamageSystem.h"

#include "GO_Profiler.h"
#include "GO_EventBroker.h"

#include "GO_HealthComponent.h"
#include "GO_Entity.h"
#include "GO_ThreadUtils.h"

namespace GO
{
	void ApplyCombatDamageInternal(const CombatDamageMessage* someCombatMessages, const size_t aStartIndex, const size_t anEndIndex, const size_t aTaskIndex, const unsigned int aTotalTaskCount, GO_APIProfiler& aProfiler)
	{
		PROFILER_SCOPED(&aProfiler, "ApplyCombatDamageInternal", 0x6B0BBFFF);
		for (size_t currentIndex = aStartIndex; currentIndex < anEndIndex; currentIndex++)
		{
			const CombatDamageMessage& damageMsg = (someCombatMessages[currentIndex]);

			Entity* dealerEntity = damageMsg.myDealerEntity;
			Entity* receiverEntity = damageMsg.myReceiverEntity;

			HealthComponent* dealerHealthComp = dealerEntity->getComponent<HealthComponent>();
			HealthComponent* receiverHealthComp = receiverEntity->getComponent<HealthComponent>();

			GO_ASSERT(receiverHealthComp, "Receiver that took damage doesn't have a health component");

			//receiverHealthComp->myHealth -= damageMsg.myDamageDealt;
			//if(receiverHealthComp->myHealth <= 0.0f)
			//{
			//	EventBroker::QueueEntityForDeath(receiverEntity, dealerEntity);
			//}
		}
	}

	void ApplyCombatDamageSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		PROFILER_SCOPED(&someUpdateParams.myProfiler, "ApplyCombatDamage", 0x8931D6FF);
		World& world = someUpdateParams.myWorld;
		ThreadPool& threadPool = someUpdateParams.myThreadPool;
		GO_APIProfiler& profiler = someUpdateParams.myProfiler;

		ThreadUtils::RunParallel(EventBroker::GetCombatMessages().begin(), EventBroker::GetCombatMessages().end(), world, threadPool, profiler, GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK, &ApplyCombatDamageInternal);

		EventBroker::ClearCombatMessages();
	}
}