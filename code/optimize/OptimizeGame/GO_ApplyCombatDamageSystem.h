#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	struct CombatDamageMessage;

	class ApplyCombatDamageSystem : public GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;
	};


	template<>
	struct GameUpdateSystemTypeTraits<ApplyCombatDamageSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::ApplyCombatDamage;
		typedef GameUpdateSystemOneDependencies<unsigned int(TaskIdentifiers::CalculateFinished)> ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<ApplyCombatDamageSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK;
	};
}