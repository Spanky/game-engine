#pragma once
#include "GO_EntityComponent.h"
#include "GO_ComponentTypes.h"

#include <array>
#include "SFML/Window/Keyboard.hpp"

namespace GO
{
	class PlayerInputComponent : public EntityComponent
	{
	public:
		static ComponentType getComponentTypeStatic() { return ComponentType::PlayerInputComponent; }

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
