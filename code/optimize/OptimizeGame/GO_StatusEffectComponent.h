#pragma once

#include "GO_EntityComponent.h"

namespace GO
{
	class StatusEffectComponent : public EntityComponent
	{
	public:
		StatusEffectComponent();
		StatusEffectComponent(Entity& anEntity);

		void performUpdate();

	private:
		int myNum;
		sf::Vector3f myScale;
		sf::Vector3f myPosition;
		sf::Vector3f myRotation;
		sf::Vector3f myRotation2;
		sf::Vector3f myRotation3;
		sf::Vector3f myRotation4;
	};
}