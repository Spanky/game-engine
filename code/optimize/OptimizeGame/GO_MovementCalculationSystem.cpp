#include "StableHeaders.h"
#include "GO_MovementCalculationSystem.h"
#include "GO_GameStateChange.h"

#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_MovementComponent.h"
#include "GO_RandomMovementComponent.h"
#include "GO_PlayerInputComponent.h"
#include "GO_Profiler.h"

namespace GO
{
	MovementCalculationSystem::MovementCalculationSystem(World& aWorld)
		: myWorld(aWorld)
	{
	}

	void MovementCalculationSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		GO_APIProfiler& profiler = someUpdateParams.myProfiler;
		PROFILER_SCOPED(&profiler, "MovementCalculationSystem", 0x00ff00ff);

		GeneratePlayerMovementBasedOnInput();
		RandomlyMoveEnemies();
	}

	void MovementCalculationSystem::RandomlyMoveEnemies()
	{
		const World::EntityList& entityList = myWorld.getEntities();
		const size_t entityListSize = entityList.size();


		// TODO: Figure out how to do random numbers better
		// Seed with a real random value, if available
		std::random_device r;

		// Choose a random mean between 0 and 1
		std::default_random_engine e1(r());
		std::uniform_int_distribution<int> uniform_dist(-1, 1);


		for (size_t outerIndex = 1; outerIndex < entityListSize; outerIndex++)
		{
			Entity* outerEntity = entityList[outerIndex];
			GO_ASSERT(outerEntity, "Entity list contains a nullptr");

			if (const RandomMovementComponent* randomMovement = outerEntity->getComponent<RandomMovementComponent>())
			{
				const MovementComponent* movementComponent = outerEntity->getComponent<MovementComponent>();
				GO_ASSERT(movementComponent, "A randomly moving entity requires a movement component to perform the movement");

				if (!movementComponent->myHasMovementQueued)
				{
					int x = uniform_dist(e1);
					int y = uniform_dist(e1);

					sf::Vector2i desiredTile = movementComponent->myCurrentTileIndex;

					int directionSelection = uniform_dist(e1);

					if (x && y)
					{
						if (directionSelection)
						{
							desiredTile.x += x;
						}
						else
						{
							desiredTile.y += y;
						}
					}
					else
					{
						desiredTile.x += x;
						desiredTile.y += y;
					}

					GameStateChange::QueueMovement(outerEntity, desiredTile);
				}
			}
		}
	}

	void MovementCalculationSystem::GeneratePlayerMovementBasedOnInput()
	{
		const World::EntityList& entityList = myWorld.getEntities();
		GO_ASSERT(entityList.size() > 0, "No entities in the world to update");

		//if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::Space))
		//{
		//	GameEvents::QueueEntityForSpawn();
		//}

		// TODO: An assumption is made that the player is the first entity in the list
		Entity* playerEntity = entityList[0];
		const MovementComponent* movementComponent = playerEntity->getComponent<MovementComponent>();
		PlayerInputComponent* playerInputComponent = playerEntity->getComponent<PlayerInputComponent>();
		

		if(!movementComponent->myHasMovementQueued)
		{
			sf::Vector2i desiredTile = movementComponent->myCurrentTileIndex;

			if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::W))
			{
				desiredTile.y--;
			}
			else if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::S))
			{
				desiredTile.y++;
			}
			else if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::D))
			{
				desiredTile.x++;
			}
			else if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::A))
			{
				desiredTile.x--;
			}


			if (desiredTile != movementComponent->myCurrentTileIndex)
			{
				GameStateChange::QueueMovement(playerEntity, desiredTile);
			}
		}
	}
}