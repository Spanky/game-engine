#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	class World;

	class MovementApplySystem : public GameUpdateSystem
	{
	public:
		MovementApplySystem(World& aWorld);

		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;

	private:
		World& myWorld;
	};


	template<>
	struct GameUpdateSystemTypeTraits<MovementApplySystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::ApplyEntityMovement;
		typedef GameUpdateSystemOneDependencies<unsigned int(TaskIdentifiers::ApplyEntitySpawns)> ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<MovementApplySystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK;
	};
}