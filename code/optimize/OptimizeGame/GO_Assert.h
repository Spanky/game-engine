#pragma once

#include <intrin.h>

#define GO_ASSERT(condition, message)								\
	{																\
		if(!(condition))											\
		{															\
			OutputDebugString((message));							\
			OutputDebugString("\n");								\
			std::cerr << (message) << std::endl;					\
			assert(condition);										\
			__debugbreak();											\
		}															\
	}