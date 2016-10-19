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
		// TODO: Figure out how to do random numbers better
		// Seed with a real random value, if available
		std::random_device r;

		// Choose a random mean between 1 and 6
		std::default_random_engine e1(r());
		std::uniform_int_distribution<int> uniform_dist(0, 1024);
		int x = uniform_dist(e1);
		int y = uniform_dist(e1);


		getEntity().setPosition(sf::Vector2i(x, y));
	}
}