#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	class ApplyEntitySpawnsSystem : public GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;
	};

	template<>
	struct GameUpdateSystemTypeTraits<ApplyEntitySpawnsSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::ApplyEntitySpawns;
		typedef GameUpdateSystemOneDependencies<unsigned int(TaskIdentifiers::ApplyEntityDeaths)> ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<ApplyEntitySpawnsSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK;
	};
}
