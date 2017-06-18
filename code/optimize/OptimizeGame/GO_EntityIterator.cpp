#include "StableHeaders.h"
#include "GO_EntityIterator.h"
#include "GO_EntityRange.h"

namespace GO
{
	EntityIterator::EntityIterator(const EntityRange& anEntityRange, size_t anEntityIndex)
		: myEntityRange(anEntityRange)
		, myEntityIndex(anEntityIndex)
	{
	}

	Entity& EntityIterator::operator*()
	{
		// TODO(scarroll): Casting away the const of this is absolutely gross
		return const_cast<Entity&>(myEntityRange[myEntityIndex]);
	}
}