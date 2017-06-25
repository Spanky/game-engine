#include "StableHeaders.h"
#include "UpdatePlayerInputSystem.h"

#include "GO_ComponentAccessControl.h"
#include "GO_PlayerInputComponent.h"

#include "GO_Profiler.h"
#include "GO_World.h"
#include "GO_Entity.h"


namespace Bricks
{
	void UpdatePlayerInputSystem::updateSystem(GO::SystemUpdateParams & someUpdateParams)
	{
		PROFILER_SCOPED(&someUpdateParams.myProfiler, "UpdateInputSystem", 0xf2ebd2FF);
		GO::ScopedComponentAccessFlags scopedAccessFlags = GO::ScopedComponentAccessFlags(GO::ReadComponentList<>(), GO::WriteComponentList<GO::PlayerInputComponent>());

		GO::World& world = someUpdateParams.myWorld;

		for(GO::Entity& currentPlayer : world.getPlayers())
		{
			GO::PlayerInputComponent* playerInputComponent = currentPlayer.getComponentWriteAccess<GO::PlayerInputComponent>(scopedAccessFlags.getRights());
			GO_ASSERT(playerInputComponent, "Player does not have an input component");

			auto& prevFrameStates = playerInputComponent->myKeyStatesPrevFrame;
			auto& thisFrameStates = playerInputComponent->myKeyStatesThisFrame;

			for (size_t currentKeyIndex = 0; currentKeyIndex < sf::Keyboard::KeyCount; currentKeyIndex++)
			{
				sf::Keyboard::Key currentKey = sf::Keyboard::Key(currentKeyIndex);

				prevFrameStates[currentKeyIndex] = thisFrameStates[currentKeyIndex];
				thisFrameStates[currentKeyIndex] = sf::Keyboard::isKeyPressed(currentKey);
			}
		}
	}
}