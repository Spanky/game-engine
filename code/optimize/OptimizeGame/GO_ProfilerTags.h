#pragma once

namespace GO_ProfilerTags
{
	const unsigned char THREAD_TAG_UNKNOWN = 0;
	const unsigned char THREAD_TAG_WAITING = 1;
	
	const unsigned char THREAD_TAG_TASK_SCHEDULER = 2;
	const unsigned char THREAD_TAG_PROPAGATION = 3;
	
	const unsigned char THREAD_TAG_PROFILER_RENDER = 4;

	const unsigned char THREAD_TAG_MAIN_THREAD = 5;

	const unsigned char THREAD_TAG_GAME_TASK = 6;
	const unsigned char THREAD_TAG_CALC_GAME_TASK = 7;
	const unsigned char THREAD_TAG_APPLY_GAME_TASK = 8;
}