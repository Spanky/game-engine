#include "StableHeaders.h"
#include "GO_MovementCalculationSystem.h"
#include "GO_GameStateChange.h"

#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_MovementComponent.h"
#include "GO_RandomMovementComponent.h"
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

		// TODO(scarroll): Setup profiler keybindings somewhere else
		if(someUpdateParams.myRenderWindow.hasFocus() && !sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem))
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::P))
			{
				someUpdateParams.myProfiler.PauseCollection();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
			{
				someUpdateParams.myProfiler.ResumeCollection();
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				profiler.ViewPrevFrame();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				profiler.ViewNextFrame();
			}
		}
	}

	void MovementCalculationSystem::RandomlyMoveEnemies()
	{
		const GO::World::EntityList& entityList = myWorld.getEntities();
		const size_t entityListSize = entityList.size();


		// TODO: Figure out how to do random numbers better
		// Seed with a real random value, if available
		std::random_device r;

		// Choose a random mean between 0 and 1
		std::default_random_engine e1(r());
		std::uniform_int_distribution<int> uniform_dist(-1, 1);


		for (size_t outerIndex = 1; outerIndex < entityListSize; outerIndex++)
		{
			GO::Entity* outerEntity = entityList[outerIndex];
			GO_ASSERT(outerEntity, "Entity list contains a nullptr");

			if (const GO::RandomMovementComponent* randomMovement = outerEntity->getComponent<GO::RandomMovementComponent>())
			{
				const GO::MovementComponent* movementComponent = outerEntity->getComponent<GO::MovementComponent>();
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

		//if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		//{
		//	GameEvents::QueueEntityForSpawn();
		//}

		// TODO: An assumption is made that the player is the first entity in the list
		Entity* playerEntity = entityList[0];
		const GO::MovementComponent* movementComponent = playerEntity->getComponent<GO::MovementComponent>();

		if(!movementComponent->myHasMovementQueued)
		{
			sf::Vector2i desiredTile = movementComponent->myCurrentTileIndex;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			{
				desiredTile.y--;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			{
				desiredTile.y++;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			{
				desiredTile.x++;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
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