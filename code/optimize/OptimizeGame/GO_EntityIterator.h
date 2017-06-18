#pragma once

namespace GO
{
	class Entity;
	class EntityRange;

	class EntityIterator
	{
	public:
		EntityIterator(const EntityRange& anEntityRange, size_t anEntityIndex);

		bool operator!=(const EntityIterator& anIterator)
		{
			// TODO(scarroll): Shouldn't we be checking the actual entity range as well?
			return myEntityIndex != anIterator.myEntityIndex;
		}

		const EntityIterator& operator++()
		{
			myEntityIndex++;
			return *this;
		}

		Entity& operator*();

	private:
		const EntityRange& myEntityRange;
		size_t myEntityIndex;
	};
}