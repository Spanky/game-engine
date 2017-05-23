#include "StableHeaders.h"
#include "GO_UpdateInputSystem.h"

#include "GO_Profiler.h"
#include "GO_World.h"
#include "GO_Entity.h"
#include "GO_PlayerInputComponent.h"

namespace GO
{
	void UpdateInputSystem::updateSystem(SystemUpdateParams& someUpdateParams)
	{
		PROFILER_SCOPED(&someUpdateParams.myProfiler, "UpdateInputSystem", 0xf2ebd2FF);
		
		World& world = someUpdateParams.myWorld;
		ThreadPool& threadPool = someUpdateParams.myThreadPool;
		GO_APIProfiler& profiler = someUpdateParams.myProfiler;

		// TODO(scarroll): An assumption that the player is always entity 0
		Entity* playerEntity = world.getEntities()[0];
		PlayerInputComponent* playerInputComponent = playerEntity->getComponent<PlayerInputComponent>();
		GO_ASSERT(playerInputComponent, "Player does not have an input component");

		auto& prevFrameStates = playerInputComponent->myKeyStatesPrevFrame;
		auto& thisFrameStates = playerInputComponent->myKeyStatesThisFrame;

		for(size_t currentKeyIndex = 0; currentKeyIndex < sf::Keyboard::KeyCount; currentKeyIndex++)
		{
			sf::Keyboard::Key currentKey = sf::Keyboard::Key(currentKeyIndex);

			prevFrameStates[currentKeyIndex] = thisFrameStates[currentKeyIndex];
			thisFrameStates[currentKeyIndex] = sf::Keyboard::isKeyPressed(currentKey);
		}
	}
}