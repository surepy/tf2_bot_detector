#pragma once

#include <cassert>

namespace mh
{
	template<typename To, typename From>
	inline constexpr To assert_cast(From f)
	{
		To sc = static_cast<To>(f);

#ifdef _DEBUG
		To dc = dynamic_cast<To>(f);
		assert(sc == dc);
#endif

		return sc;
	}
}
