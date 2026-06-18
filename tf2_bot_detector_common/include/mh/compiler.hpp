#pragma once

namespace mh
{
	constexpr bool is_debug =
#if defined(DEBUG) || defined(_DEBUG)
		true;
#else
		false;
#endif

	constexpr bool is_msvc =
#ifdef _MSC_VER
		true;
#else
		false;
#endif

	constexpr bool is_32bit = sizeof(void*) == 4;
	constexpr bool is_64bit = sizeof(void*) == 8;
}
