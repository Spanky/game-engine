#pragma once

namespace GO
{
	class Entity;


	class EntityComponent
	{
	public:
		EntityComponent(Entity& anEntity);

		virtual ~EntityComponent() {};

	protected:
		Entity& getEntity() { return myEntity; }

	private:
		Entity& myEntity;
	};
}