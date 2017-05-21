#pragma once

#include "GO_ComponentUpdater.h"

namespace GO
{
	class EntityComponent;

	class TaskScheduler
	{
	public:
		void registerComponent(EntityComponent* aComponent);
		void unregisterComponent(EntityComponent* aComponent);

	private:
		ComponentUpdater myComponentUpdater;
	};
}