#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	class DebugInputSystem : public GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) override;
	};


	template<>
	struct GameUpdateSystemTypeTraits<DebugInputSystem>
	{
		static constexpr TaskIdentifiers ourTaskIdentifier = TaskIdentifiers::DebugInput;
		typedef GameUpdateSystemOneDependencies<unsigned int(TaskIdentifiers::UpdateInput)> ourTaskDependencies;
	};

	template<>
	struct ProfilerTypeTraits<DebugInputSystem>
	{
		static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK;
	};
}