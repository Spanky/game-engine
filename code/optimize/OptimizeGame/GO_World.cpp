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
		delete anEntityToDestroy;
	}
}