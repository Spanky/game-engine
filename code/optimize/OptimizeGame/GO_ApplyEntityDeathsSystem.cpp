#include "StableHeaders.h"
#include "GO_ApplyEntityDeathsSystem.h"

#include "GO_Profiler.h"
#include "GO_EventBroker.h"

#include "GO_World.h"
#include "GO_Entity.h"

namespace GO
{
	void ApplyEntityDeathsSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		PROFILER_SCOPED(&someUpdateParams.myProfiler, "ApplyEntityDeaths", 0xFFFFFFFF);
		GO::World& world = someUpdateParams.myWorld;
		const GO::World::EntityList& entityList = world.getEntities();

		for (const GO::EntityDeathMessage& deathMsg : GO::EventBroker::GetDeathMessages())
		{
			const size_t deadEntityId = deathMsg.myDeadEntityId;

			GO::World::EntityList::const_iterator deadIter = std::find_if(entityList.begin(), entityList.end(),
				[deadEntityId](const GO::Entity* anEntity)
			{
				return anEntity->getId() == deadEntityId;
			});

			// We may have already destroyed this entity so don't bother destroying it again
			// (Multiple things could have all killed this entity in the same frame)
			if (deadIter != entityList.end())
			{
				world.destroyEntity(*deadIter);
			}
		}

		GO::EventBroker::ClearDeathMessages();
	}
}