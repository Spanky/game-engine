#pragma once

namespace GO_ProfilerTags
{
	const unsigned char THREAD_TAG_UNKNOWN = 0;
	const unsigned char THREAD_TAG_WAITING = 1;
	const unsigned char THREAD_TAG_OVERHEAD = 2;
	
	const unsigned char THREAD_TAG_TASK_SCHEDULER = 3;
	const unsigned char THREAD_TAG_PROPAGATION = 4;
	
	const unsigned char THREAD_TAG_PROFILER_RENDER = 5;

	const unsigned char THREAD_TAG_MAIN_THREAD = 6;

	const unsigned char THREAD_TAG_GAME_TASK = 7;
	const unsigned char THREAD_TAG_CALC_GAME_TASK = 8;
	const unsigned char THREAD_TAG_CALC_SYNC_POINT_TASK = 9;

	const unsigned char THREAD_TAG_APPLY_GAME_TASK = 10;
	const unsigned char THREAD_TAG_APPLY_SYNC_POINT_TASK = 11;

	// NOTE: Update this each time the thread tags are updated
	const unsigned char THREAD_TAG_COUNT = THREAD_TAG_APPLY_SYNC_POINT_TASK + 1;
}