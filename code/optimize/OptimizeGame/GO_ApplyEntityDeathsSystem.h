#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	class ApplyEntityDeathsSystem : public GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;
	};


	template<>
	struct GameUpdateSystemTypeTraits<ApplyEntityDeathsSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::ApplyEntityDeaths;
		typedef GameUpdateSystemOneDependencies<unsigned int(TaskIdentifiers::ApplyCombatDamage)> ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<ApplyEntityDeathsSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK;
	};
}