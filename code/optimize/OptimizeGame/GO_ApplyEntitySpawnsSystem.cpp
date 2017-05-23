#include "StableHeaders.h"
#include "GO_ApplyEntitySpawnsSystem.h"

#include "GO_Profiler.h"
#include "GO_EventBroker.h"

#include "GO_World.h"
#include "GO_Entity.h"

#include "GO_SpriteComponent.h"
#include "GO_RandomMovementComponent.h"
#include "GO_HealthComponent.h"

// TODO(scarroll): This is required for the createComponent call that registers components with the legacy task scheduler
#include "GO_TaskScheduler.h"
#include "GO_GameInstance.h"

namespace GO
{
	void ApplyEntitySpawnsSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		GO::World& world = someUpdateParams.myWorld;

		for (const GO::EntitySpawnMessage& spawnMsg : GO::EventBroker::GetSpawnMessages())
		{
			GO::Entity* entity = world.createEntity();
			entity->createComponent<GO::SpriteComponent>();
			entity->createComponent<GO::RandomMovementComponent>();
			entity->createComponent<GO::HealthComponent>();
		}

		GO::EventBroker::ClearSpawnMessages();
	}
}