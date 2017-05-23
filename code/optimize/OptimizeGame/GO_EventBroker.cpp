#include "StableHeaders.h"
#include "GO_EventBroker.h"

#include "GO_Entity.h"
#include "GO_Profiler.h"

namespace GO
{
	namespace EventBroker
	{
		std::vector<CombatDamageMessage> ourCombatDamageMessages;
		std::vector<EntityDeathMessage> ourEntityDeathMessages;
		std::vector<EntitySpawnMessage> ourEntitySpawnMessages;

		std::mutex ourDamageQueueMutex;

		void QueueDamage(GO::Entity* aDealer, GO::Entity* aReceiver, const float aDamageDone)
		{
			std::lock_guard<std::mutex> lock(ourDamageQueueMutex);

			CombatDamageMessage msg;
			msg.myDealerEntity = aDealer;
			msg.myReceiverEntity = aReceiver;
			msg.myDamageDealt = aDamageDone;

			ourCombatDamageMessages.push_back(msg);
		}

		void QueueDamage(const std::vector<CombatDamageMessage>& someDamageMessages, GO_APIProfiler& aProfiler)
		{
			PROFILER_SCOPED(&aProfiler, "QueueDamage", 0xd986e8ff);
			std::lock_guard<std::mutex> lock(ourDamageQueueMutex);
			ourCombatDamageMessages.insert(ourCombatDamageMessages.end(), someDamageMessages.begin(), someDamageMessages.end());
		}

		void QueueEntityForDeath(const GO::Entity* aDeadEntity, const GO::Entity* aKillerEntity)
		{
			EntityDeathMessage msg;
			msg.myDeadEntityId = aDeadEntity->getId();
			msg.myKillerEntityId = aKillerEntity->getId();

			ourEntityDeathMessages.push_back(msg);
		}

		void QueueEntityForSpawn()
		{
			EntitySpawnMessage msg;

			ourEntitySpawnMessages.push_back(msg);
		}

		std::vector<CombatDamageMessage>& GetCombatMessages()
		{
			return ourCombatDamageMessages;
		}

		void ClearCombatMessages()
		{
			ourCombatDamageMessages.clear();
		}

		std::vector<EntityDeathMessage>& GetDeathMessages()
		{
			return ourEntityDeathMessages;
		}

		void ClearDeathMessages()
		{
			ourEntityDeathMessages.clear();
		}

		std::vector<EntitySpawnMessage>& GetSpawnMessages()
		{
			return ourEntitySpawnMessages;
		}

		void ClearSpawnMessages()
		{
			ourEntitySpawnMessages.clear();
		}
	}
}