#include "StableHeaders.h"
#include "GO_RandomMovementComponent.h"

#include "GO_Entity.h"

namespace GO
{
	RandomMovementComponent::RandomMovementComponent(Entity& anEntity)
		: EntityComponent(anEntity)
	{
	}

	void RandomMovementComponent::update()
	{
		getEntity().setPosition(sf::Vector2i(std::rand() % 1024, std::rand() % 1024));
	}
}