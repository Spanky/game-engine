#pragma once

#include "GO_EntityIterator.h"

namespace GO
{
	class Entity;

	class EntityRange
	{
	public:
		EntityRange(Entity** someEntities, size_t anEntityCount);

		// TODO(scarroll): Provide a 'ConstEntityIterator'
		EntityIterator begin() const { return EntityIterator(*this, 0); }
		EntityIterator end() const { return EntityIterator(*this, myEntityCount); }

		const Entity& operator[](const size_t anIndex) const;
		Entity& operator[](const size_t anIndex);

	private:
		Entity** myEntities;
		size_t myEntityCount;
	};
}