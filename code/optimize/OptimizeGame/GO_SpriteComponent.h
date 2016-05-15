#pragma once

#include "GO_EntityComponent.h"

namespace GO
{
	class SpriteComponent : public EntityComponent
	{
	public:
		SpriteComponent(Entity& anEntity);

	private:
		virtual void update() override;

		sf::Sprite mySprite;
	};
}