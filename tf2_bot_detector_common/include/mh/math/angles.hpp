#pragma once

#include <type_traits>

namespace mh
{
	template<typename T, typename TResult = std::common_type_t<T, float>>
	constexpr TResult deg2rad(T degrees)
	{
		return TResult(degrees) * TResult(0.01745329251994329577);
	}

	template<typename T, typename TResult = std::common_type_t<T, float>>
	constexpr TResult rad2deg(T radians)
	{
		return TResult(radians) * TResult(57.2957795130823208768);
	}
}
