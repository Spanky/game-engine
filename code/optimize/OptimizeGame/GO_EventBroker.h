#pragma once

class GO_APIProfiler;

namespace GO
{
	class Entity;


	// TODO(scarroll): Put these messages somewhere that won't be included by everyone who deals with game events
	struct CombatDamageMessage
	{
		Entity* myDealerEntity;
		Entity* myReceiverEntity;
		float myDamageDealt;
	};

	struct EntityDeathMessage
	{
		// TODO: Replace these with handles eventually to avoid the constant lookups all the time
		size_t myDeadEntityId;
		size_t myKillerEntityId;
	};

	struct EntitySpawnMessage
	{
	};



	namespace EventBroker
	{
		void QueueDamage(Entity* aDealer, Entity* aReceiver, const float aDamageDone);
		void QueueDamage(const std::vector<CombatDamageMessage>& someDamageMessages, GO_APIProfiler& aProfiler);
		void QueueEntityForDeath(const GO::Entity* aDeadEntity, const GO::Entity* aKillerEntity);
		void QueueEntityForSpawn();

		std::vector<CombatDamageMessage>& GetCombatMessages();
		void ClearCombatMessages();

		std::vector<EntityDeathMessage>& GetDeathMessages();
		void ClearDeathMessages();

		std::vector<EntitySpawnMessage>& GetSpawnMessages();
		void ClearSpawnMessages();
	}
}