#include "StableHeaders.h"
#include "GO_ComponentUpdater.h"

namespace GO
{
	void ComponentUpdater::registerComponent(EntityComponent* aComponent)
	{
		myComponents.push_back(aComponent);
	}

	void ComponentUpdater::unregisterComponent(EntityComponent* aComponent)
	{
		const auto searchIter = std::find(myComponents.begin(), myComponents.end(), aComponent);
		GO_ASSERT(searchIter != myComponents.end(), "Component was not registered with the updater system");

		myComponents.erase(searchIter);
	}
}