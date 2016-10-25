#pragma once

#include "GO_EntityComponent.h"

namespace GO
{
	class MovementComponent : public EntityComponent
	{
	public:
		MovementComponent(Entity& anEntity)
			: EntityComponent(anEntity)
			, myCurrentTileIndex(0, 0)
			, myDestinationTileIndex(0, 0)
			, myTileMovementCursor(0.0f)
			, myHasMovementQueued(false)
		{
		}

		sf::Vector2i myCurrentTileIndex;
		sf::Vector2i myDestinationTileIndex;
		float myTileMovementCursor;

		// TODO: How do we track things like having already queued up a movement request for this entity?
		bool myHasMovementQueued;

	private:
		virtual void update() override;
	};
}