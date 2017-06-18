#include "StableHeaders.h"
#include "GO_EntityRange.h"

namespace GO
{
	EntityRange::EntityRange(Entity** someEntities, size_t anEntityCount)
		: myEntities(someEntities)
		, myEntityCount(anEntityCount)
	{
	}

	const Entity& EntityRange::operator[](const size_t anIndex) const
	{
		GO_ASSERT(anIndex >= 0 && anIndex < myEntityCount, "Index out of bounds");
		return *myEntities[anIndex];
	}

	Entity& EntityRange::operator[](const size_t anIndex)
	{
		GO_ASSERT(anIndex >= 0 && anIndex < myEntityCount, "Index out of bounds");
		return *myEntities[anIndex];
	}
}