#include "StableHeaders.h"
#include "GO_World.h"
#include "GO_Entity.h"

namespace GO
{
	World::World()
	{
	}

	Entity* World::createEntity()
	{
		Entity* newEntity = new Entity();
		myEntities.push_back(newEntity);

		return newEntity;
	}

	void World::destroyEntity(Entity* anEntityToDestroy)
	{
		EntityList::iterator iter = std::find(myEntities.begin(), myEntities.end(), anEntityToDestroy);
		GO_ASSERT(iter != myEntities.end(), "Could not find entity to destroy");

		myEntities.erase(iter);

		anEntityToDestroy->destroyAllComponents();
		delete anEntityToDestroy;
	}

	EntityRange World::getPlayers()
	{
		// TODO(scarroll): There is an assumption that the players are stored at index [0]
		GO_ASSERT(myEntities.size() > 0, "There are no players yet");
		return EntityRange(myEntities.data(), 1);
	}
}