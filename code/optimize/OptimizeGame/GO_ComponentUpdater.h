#pragma once

namespace GO
{
	class EntityComponent;

	class ComponentUpdater
	{
	public:
		void registerComponent(EntityComponent* aComponent);
		void unregisterComponent(EntityComponent* aComponent);

		void updateComponents();

	private:
		std::vector<EntityComponent*> myComponents;

		//std::vector<sf::Sprite> mySprites;
	};
}