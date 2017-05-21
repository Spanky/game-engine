#pragma once

namespace GO_ProfilerTags
{
	const unsigned char THREAD_TAG_UNKNOWN = 0;
	const unsigned char THREAD_TAG_IDLE = THREAD_TAG_UNKNOWN + 1;
	const unsigned char THREAD_TAG_WAITING = THREAD_TAG_IDLE + 1;
	const unsigned char THREAD_TAG_OVERHEAD = THREAD_TAG_WAITING + 1;
	
	const unsigned char THREAD_TAG_TASK_SCHEDULER = THREAD_TAG_OVERHEAD + 1;
	const unsigned char THREAD_TAG_PROPAGATION = THREAD_TAG_TASK_SCHEDULER + 1;
	
	const unsigned char THREAD_TAG_PROFILER_RENDER = THREAD_TAG_PROPAGATION + 1;

	const unsigned char THREAD_TAG_MAIN_THREAD = THREAD_TAG_PROFILER_RENDER + 1;

	const unsigned char THREAD_TAG_GAME_TASK = THREAD_TAG_MAIN_THREAD + 1;
	const unsigned char THREAD_TAG_CALC_GAME_TASK = THREAD_TAG_GAME_TASK + 1;
	const unsigned char THREAD_TAG_CALC_SYNC_POINT_TASK = THREAD_TAG_CALC_GAME_TASK + 1;

	const unsigned char THREAD_TAG_APPLY_GAME_TASK = THREAD_TAG_CALC_SYNC_POINT_TASK + 1;
	const unsigned char THREAD_TAG_APPLY_SYNC_POINT_TASK = THREAD_TAG_APPLY_GAME_TASK + 1;

	// NOTE: Update this each time the thread tags are updated
	const unsigned char THREAD_TAG_COUNT = THREAD_TAG_APPLY_SYNC_POINT_TASK + 1;
}