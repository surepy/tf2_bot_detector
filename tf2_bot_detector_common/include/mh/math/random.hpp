#pragma once

#include <cstdint>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh
{
	template<typename T>
	MH_STUFF_API T get_random(T min_inclusive, T max_inclusive);

#ifdef MH_COMPILE_LIBRARY
	extern template MH_STUFF_API float get_random(float, float);
	extern template MH_STUFF_API double get_random(double, double);

	extern template MH_STUFF_API int8_t get_random(int8_t, int8_t);
	extern template MH_STUFF_API uint8_t get_random(uint8_t, uint8_t);
	extern template MH_STUFF_API int16_t get_random(int16_t, int16_t);
	extern template MH_STUFF_API uint16_t get_random(uint16_t, uint16_t);
	extern template MH_STUFF_API int32_t get_random(int32_t, int32_t);
	extern template MH_STUFF_API uint32_t get_random(uint32_t, uint32_t);
	extern template MH_STUFF_API int64_t get_random(int64_t, int64_t);
	extern template MH_STUFF_API uint64_t get_random(uint64_t, uint64_t);
#endif
}

#ifndef MH_COMPILE_LIBRARY
#include "random.inl"
#endif
