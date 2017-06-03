#include "StableHeaders.h"
#include "GO_DebugInputSystem.h"

#include "GO_Profiler.h"
#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_PlayerInputComponent.h"
#include "GO_ComponentAccessControl.h"

namespace GO
{
	void DebugInputSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		PROFILER_SCOPED(&someUpdateParams.myProfiler, "DebugInputSystem", 0x52B230FF);
		World& world = someUpdateParams.myWorld;
		GO_APIProfiler& profiler = someUpdateParams.myProfiler;

		auto accessFlags = ComponentAccessControl::requestAccess(ReadComponentList<PlayerInputComponent>(), WriteComponentList<>());

		// TODO(scarroll): An assumption that the player is always entity 0
		Entity* playerEntity = world.getEntities()[0];
		const PlayerInputComponent* playerInputComponent = playerEntity->getComponentReadAccess<PlayerInputComponent>(accessFlags);
		GO_ASSERT(playerInputComponent, "Player does not have an input component");

		if (someUpdateParams.myRenderWindow.hasFocus() && !playerInputComponent->myKeyStatesThisFrame[sf::Keyboard::LSystem])
		{
			if(playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::P))
			{
				someUpdateParams.myProfiler.PauseCollection();
			}
			else if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::R))
			{
				someUpdateParams.myProfiler.ResumeCollection();
			}

			if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::Left))
			{
				profiler.ViewPrevFrame();
			}
			else if (playerInputComponent->wasKeyPressedThisFrame(sf::Keyboard::Right))
			{
				profiler.ViewNextFrame();
			}
		}

		ComponentAccessControl::releaseAccess(accessFlags);
	}
}