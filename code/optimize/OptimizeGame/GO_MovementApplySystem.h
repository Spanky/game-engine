#pragma once

namespace GO
{
	class World;

	class MovementApplySystem
	{
	public:
		MovementApplySystem(World& aWorld);

		void ApplyPendingMovementChanges(sf::Time aDeltaTime);

	private:
		World& myWorld;
	};
}