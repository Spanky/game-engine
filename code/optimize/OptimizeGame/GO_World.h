#pragma once

namespace GO
{
	class Entity;

	class World
	{
	public:
		typedef std::vector<Entity*> EntityList;

	public:
		World();

		Entity* createEntity();
		void destroyEntity(Entity* anEntityToDestroy);

		EntityList& getEntities() { return myEntities; }

	private:
		EntityList myEntities;
	};
}