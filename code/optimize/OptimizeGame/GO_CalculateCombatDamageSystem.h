#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	class CalculateCombatDamageSystem : public GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;
	};


	template<>
	struct GameUpdateSystemTypeTraits<CalculateCombatDamageSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::CalculateCombat;
		typedef GameUpdateSystemNoDependencies ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<CalculateCombatDamageSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK;
	};
}