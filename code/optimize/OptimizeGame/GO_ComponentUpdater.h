#pragma once

namespace GO
{
	class EntityComponent;

	class ComponentUpdater
	{
	public:
		void registerComponent(EntityComponent* aComponent);
		void unregisterComponent(EntityComponent* aComponent);

	private:
		std::vector<EntityComponent*> myComponents;
	};
}