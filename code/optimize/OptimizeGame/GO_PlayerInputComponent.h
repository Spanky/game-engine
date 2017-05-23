#pragma once
#include "GO_EntityComponent.h"

#include <array>
#include "SFML/Window/Keyboard.hpp"

namespace GO
{
	class PlayerInputComponent : public EntityComponent
	{
	public:
		PlayerInputComponent(Entity& anEntity)
			: EntityComponent(anEntity)
		{
		}

		bool wasKeyPressedThisFrame(sf::Keyboard::Key aKey) const
		{
			return myKeyStatesThisFrame[aKey] && !myKeyStatesPrevFrame[aKey];
		}

		std::array<bool, sf::Keyboard::KeyCount> myKeyStatesThisFrame;
		std::array<bool, sf::Keyboard::KeyCount> myKeyStatesPrevFrame;
	};
}
