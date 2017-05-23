#pragma once

#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	class UpdateInputSystem : public GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;
	};


	template<>
	struct GameUpdateSystemTypeTraits<UpdateInputSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::UpdateInput;
		typedef GameUpdateSystemNoDependencies ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<UpdateInputSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK;
	};
}