#pragma once


namespace GO
{
	class World;

	class MovementCalculationSystem
	{
	public:
		MovementCalculationSystem(World& aWorld);

		void GenerateCalculations();

	private:
		void GeneratePlayerMovementBasedOnInput();
		void RandomlyMoveEnemies();

	private:
		World& myWorld;
	};
}