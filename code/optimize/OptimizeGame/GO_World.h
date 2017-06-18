#pragma once

#include "GO_EntityRange.h"

namespace GO
{
	class Entity;

	class World
	{
	public:
		typedef std::vector<Entity*> EntityList;

	public:
		World();

		World(const World& aRHS) = delete;
		World& operator=(const World& aRHS) = delete;

		Entity* createEntity();
		void destroyEntity(Entity* anEntityToDestroy);

		EntityList& getEntities() { return myEntities; }

		EntityRange getPlayers();

	private:
		EntityList myEntities;
	};
}