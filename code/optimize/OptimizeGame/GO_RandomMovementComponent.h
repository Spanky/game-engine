#pragma once

#include "GO_EntityComponent.h"
#include "GO_ComponentTypes.h"

namespace GO
{
	class RandomMovementComponent : public EntityComponent
	{
	public:
		static ComponentType getComponentTypeStatic() { return ComponentType::RandomMovementComponent; }

		RandomMovementComponent(Entity& anEntity)
			: EntityComponent(anEntity)
		{
		}
	};
}