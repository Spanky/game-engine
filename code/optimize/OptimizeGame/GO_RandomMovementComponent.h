#pragma once

#include "GO_EntityComponent.h"

namespace GO
{
	class RandomMovementComponent : public EntityComponent
	{
	public:
		RandomMovementComponent(Entity& anEntity);

	private:
		virtual void update() override;
	};
}