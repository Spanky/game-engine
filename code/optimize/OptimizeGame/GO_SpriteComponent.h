#pragma once

#include "GO_EntityComponent.h"
#include "GO_ComponentTypes.h"

namespace GO
{
	class SpriteComponent : public EntityComponent
	{
	public:
		static ComponentType getComponentTypeStatic() { return ComponentType::SpriteComponent; }

		SpriteComponent(Entity& anEntity);

		void render(sf::RenderWindow& aRenderWindow);

	private:
		sf::Sprite mySprite;
	};
}