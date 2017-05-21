#include "StableHeaders.h"
#include "GO_SpriteComponent.h"

#include "GO_Entity.h"
#include "GO_GameInstance.h"
#include "GO_HacksGlobalResources.h"

namespace GO
{
	SpriteComponent::SpriteComponent(Entity& anEntity)
		: EntityComponent(anEntity)
	{
		sf::Texture& linkTexture = GameInstance::GetInstance()->getHacksGlobalResources().getLinkSpriteTexture();
		
		mySprite.setTexture(linkTexture);
		mySprite.setTextureRect(sf::IntRect(0, 7, 26, 26));
		mySprite.setScale(5.0f, 5.0f);
	}

	void SpriteComponent::render(sf::RenderWindow& aRenderWindow)
	{
		sf::Vector2f spritePosition(getEntity().getPosition().x, getEntity().getPosition().y);
		mySprite.setPosition(spritePosition);

		aRenderWindow.draw(mySprite);
	}
}