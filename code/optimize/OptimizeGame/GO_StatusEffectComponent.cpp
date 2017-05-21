#include "StableHeaders.h"
#include "GO_StatusEffectComponent.h"

#include "GO_Entity.h"

static GO::Entity theEntity;

namespace GO
{
	StatusEffectComponent::StatusEffectComponent()
		: EntityComponent(theEntity)
		, myNum(0)
	{
	}

	StatusEffectComponent::StatusEffectComponent(Entity& anEntity)
		: EntityComponent(anEntity)
		, myNum(0)
	{
	}

	void StatusEffectComponent::performUpdate()
	{
		myNum++;

		myScale = sf::Vector3f(myNum, myNum / 2, myNum / 4);
		myPosition = sf::Vector3f(myNum / 15, myNum / 24, myNum / 32);
		myRotation = sf::Vector3f(myNum / 1.5f, myNum / 8, myNum / 16);
		myRotation2 = sf::Vector3f(myNum / 13, myNum / 12, myNum / 62);
		myRotation3 = sf::Vector3f(myNum / 23, myNum / 22, myNum / 42);
		myRotation4 = sf::Vector3f(myNum / 29, myNum / 29, myNum / 49);
	}
}