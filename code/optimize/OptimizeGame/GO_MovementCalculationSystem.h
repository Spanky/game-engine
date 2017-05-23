#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"


namespace GO
{
	class World;

	class MovementCalculationSystem : public GameUpdateSystem
	{
	public:
		MovementCalculationSystem(World& aWorld);

		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;

	private:
		void GeneratePlayerMovementBasedOnInput();
		void RandomlyMoveEnemies();

	private:
		World& myWorld;
	};


	template<>
	struct GameUpdateSystemTypeTraits<MovementCalculationSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::CalculateMovement;
		typedef GameUpdateSystemOneDependencies<unsigned int(TaskIdentifiers::UpdateInput)> ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<MovementCalculationSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK;
	};
}