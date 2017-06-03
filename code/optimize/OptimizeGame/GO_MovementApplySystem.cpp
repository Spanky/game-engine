#include "StableHeaders.h"
#include "GO_MovementApplySystem.h"
#include "GO_GameStateChange.h"

#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_MovementComponent.h"
#include "GO_HealthComponent.h"
#include "GO_Profiler.h"
#include "GO_ComponentAccessControl.h"

namespace
{
	sf::Vector2i LerpTiles(const sf::Vector2i& aCurrentTile, const sf::Vector2i& aDestinationTile, const float aLerpCursor)
	{
		sf::Vector2f startPosition = sf::Vector2f(32.0f * aCurrentTile.x, 32.0f * aCurrentTile.y);
		sf::Vector2f endPosition = sf::Vector2f(32.0f * aDestinationTile.x, 32.0f * aDestinationTile.y);

		sf::Vector2f interpPosition(
			startPosition.x + aLerpCursor * (endPosition.x - startPosition.x),
			startPosition.y + aLerpCursor * (endPosition.y - startPosition.y));

		sf::Vector2i position = sf::Vector2i(interpPosition.x, interpPosition.y);
		return position;
	}
}

namespace GO
{
	MovementApplySystem::MovementApplySystem(World& aWorld)
		: myWorld(aWorld)
	{
	}

	void MovementApplySystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		GO_APIProfiler& profiler = someUpdateParams.myProfiler;
		PROFILER_SCOPED(&profiler, "MovementApplySystem", 0xff00ffff);

		ComponentAccessFlagsType accessFlags = ComponentAccessControl::requestAccess(ReadComponentList<>(), WriteComponentList<MovementComponent>());

		const sf::Time deltaTime = someUpdateParams.myDeltaTime;

		// Process all of our messages before we update the actual movement
		for (const GameStateChange::MovementMessage& movementMsg : GameStateChange::ourMovementMessages)
		{
			GO::MovementComponent* movementComponent = movementMsg.myEntityToMove->getComponentWriteAccess<GO::MovementComponent>(accessFlags);

			GO_ASSERT(movementComponent, "Entity received a movement message but has no movement component");
			GO_ASSERT(!movementComponent->myHasMovementQueued, "Entity already has a movement request outstanding");
			movementComponent->myHasMovementQueued = true;

			movementComponent->myDestinationTileIndex = movementMsg.myTileToMoveTo;
		}

		// TODO: We should probably be able to keep a list of entities that we know are moving so we don't have to
		//		 constantly check all entities in the world
		const GO::World::EntityList& entityList = myWorld.getEntities();
		const size_t entityListSize = entityList.size();

		for (size_t entityIndex = 0; entityIndex < entityListSize; entityIndex++)
		{
			GO::Entity* currentEntity = entityList[entityIndex];
			GO::MovementComponent* movementComponent = currentEntity->getComponentWriteAccess<GO::MovementComponent>(accessFlags);
			if (movementComponent)
			{
				if (movementComponent->myHasMovementQueued)
				{
					movementComponent->myTileMovementCursor += (deltaTime.asSeconds() * movementComponent->myTilesPerSecond);

					sf::Vector2i lerpedPosition = LerpTiles(movementComponent->myCurrentTileIndex, movementComponent->myDestinationTileIndex, movementComponent->myTileMovementCursor);
					currentEntity->setPosition(lerpedPosition);

					if (movementComponent->myTileMovementCursor >= 1.0f)
					{
						movementComponent->myTileMovementCursor = 0;
						movementComponent->myCurrentTileIndex = movementComponent->myDestinationTileIndex;
						movementComponent->myHasMovementQueued = false;
					}
				}
			}
		}

		GameStateChange::ourMovementMessages.clear();

		ComponentAccessControl::releaseAccess(accessFlags);
	}
}