#include "StableHeaders.h"
#include "GO_EntityComponent.h"

namespace GO
{
	EntityComponent::EntityComponent(Entity& anEntity)
		: myEntity(anEntity)
	{
	}

	void EntityComponent::updateComponent()
	{
		update();
	}
}