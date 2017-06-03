#pragma once
#include "GO_EntityComponent.h"
#include "GO_ComponentTypes.h"

namespace GO
{
	class HealthComponent : public EntityComponent
	{
	public:
		static ComponentType getComponentTypeStatic() { return ComponentType::HealthComponent; }

		HealthComponent(Entity& anEntity)
			: EntityComponent(anEntity)
			, myHealth(1000.0f)
		{
		}

		float myHealth;
	};
}
