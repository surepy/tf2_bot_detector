#pragma once

#include <cassert>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh
{
#ifdef _WIN32
	// Gets the upper and lower bounds of the current thread's stack
	MH_STUFF_API void get_current_thread_stack_range(void*& lower, void*& upper);

	MH_STUFF_API bool is_variable_on_current_stack(const void* var);

	// Checks if a variable is on the current thread's stack
	template<typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
	inline bool is_variable_on_current_stack(const T& var)
	{
		return is_variable_on_current_stack((const void*)&var);
	}
	template<typename T>
	inline bool is_variable_on_current_stack(const T* var)
	{
		return is_variable_on_current_stack((const void*)var);
	}
#endif
}

#ifndef MH_COMPILE_LIBRARY
#include "stack_info.inl"
#endif
