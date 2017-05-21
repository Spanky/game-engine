#pragma once
#include "GO_EntityComponent.h"

namespace GO
{
	class HealthComponent : public EntityComponent
	{
	public:
		HealthComponent(Entity& anEntity)
			: EntityComponent(anEntity)
			, myHealth(1000.0f)
		{
		}

		float myHealth;
	};
}
