#pragma once

#include "GO_EntityComponent.h"

namespace GO
{
	class SpriteComponent : public EntityComponent
	{
	public:
		SpriteComponent(Entity& anEntity);

		void render(sf::RenderWindow& aRenderWindow);

	private:
		sf::Sprite mySprite;
	};
}