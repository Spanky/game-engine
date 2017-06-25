#pragma once

#include "GO_GameUpdateSystem.h"
#include "GO_TaskIdentifiers.h"
#include "GO_ProfilerTags.h"

namespace Bricks
{
	class UpdatePlayerInputSystem : public GO::GameUpdateSystem
	{
	public:
		virtual void updateSystem(GO::SystemUpdateParams& someUpdateParams);
	};
}


template<>
struct GO::GameUpdateSystemTypeTraits<Bricks::UpdatePlayerInputSystem>
{
	static constexpr GO::TaskIdentifiers ourTaskIdentifier = GO::TaskIdentifiers::UpdatePlayerInput;
	typedef GO::GameUpdateSystemNoDependencies ourTaskDependencies;
};

template<>
struct GO::ProfilerTypeTraits<Bricks::UpdatePlayerInputSystem>
{
	static const unsigned char ourProfilerTag = GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK;
};