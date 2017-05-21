#include "StableHeaders.h"
#include "GO_TaskScheduler.h"

namespace GO
{
	void TaskScheduler::registerComponent(EntityComponent* aComponent)
	{
		myComponentUpdater.registerComponent(aComponent);
	}

	void TaskScheduler::unregisterComponent(EntityComponent* aComponent)
	{
		myComponentUpdater.unregisterComponent(aComponent);
	}
}