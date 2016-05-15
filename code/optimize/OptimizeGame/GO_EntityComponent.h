#pragma once

namespace GO
{
	class Entity;


	class EntityComponent
	{
	public:
		EntityComponent(Entity& anEntity);

		virtual ~EntityComponent() {};

		void updateComponent();

	protected:
		Entity& getEntity() { return myEntity; }

	private:
		virtual void update() = 0;

	private:
		Entity& myEntity;
	};
}