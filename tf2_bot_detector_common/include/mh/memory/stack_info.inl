#ifdef MH_COMPILE_LIBRARY
#include "stack_info.hpp"
#else
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

#ifdef _WIN32
#include <Windows.h>
#include <processthreadsapi.h>

namespace mh
{
	MH_COMPILE_LIBRARY_INLINE void get_current_thread_stack_range(void*& lower, void*& upper)
	{
		ULONG_PTR tempLower, tempUpper;
		GetCurrentThreadStackLimits(&tempLower, &tempUpper);
		lower = (void*)tempLower;
		upper = (void*)tempUpper;
	}

	MH_COMPILE_LIBRARY_INLINE bool is_variable_on_current_stack(const void* var)
	{
		void* lower;
		void* upper;
		get_current_thread_stack_range(lower, upper);

		assert(lower < upper);
		return var >= lower && var <= upper;
	}
}
#endif
