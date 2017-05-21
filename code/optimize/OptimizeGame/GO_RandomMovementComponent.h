#pragma once

#include "GO_EntityComponent.h"

namespace GO
{
	class RandomMovementComponent : public EntityComponent
	{
	public:
		RandomMovementComponent(Entity& anEntity)
			: EntityComponent(anEntity)
		{
		}
	};
}