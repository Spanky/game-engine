#include "StableHeaders.h"
#include "GO_TaskScheduler.h"
#include "GO_Entity.h"

namespace GO
{
	void TaskScheduler::update()
	{
		myComponentUpdater.updateComponents();
	}

	void TaskScheduler::registerComponent(EntityComponent* aComponent)
	{
		myComponentUpdater.registerComponent(aComponent);
	}

	void TaskScheduler::unregisterComponent(EntityComponent* aComponent)
	{
		myComponentUpdater.unregisterComponent(aComponent);
	}
}