#pragma once

#include "GO_ComponentUpdater.h"

namespace GO
{
	class EntityComponent;

	class TaskScheduler
	{
	public:
		void update();

		void registerComponent(EntityComponent* aComponent);

	private:
		ComponentUpdater myComponentUpdater;
	};
}