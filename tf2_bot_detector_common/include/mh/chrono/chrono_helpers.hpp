#pragma once

#include <chrono>
#include <ctime>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh::chrono
{
	template<typename T = double, typename TRep, typename TPeriod>
	inline constexpr T to_seconds(const std::chrono::duration<TRep, TPeriod>& duration)
	{
		return std::chrono::duration_cast<std::chrono::duration<T>>(duration).count();
	}

	enum class time_zone
	{
		local = -100000,
		utc = 0,
	};

	MH_STUFF_API std::time_t to_time_t(const std::chrono::system_clock::time_point& t);
	MH_STUFF_API std::time_t to_time_t(std::tm t, time_zone zone);

	MH_STUFF_API std::tm to_tm(const std::chrono::system_clock::time_point& t, time_zone zone);
	MH_STUFF_API std::tm to_tm(const std::time_t& t, time_zone zone);

	MH_STUFF_API std::chrono::system_clock::time_point to_time_point(const std::tm& t, time_zone zone);
	MH_STUFF_API std::chrono::system_clock::time_point to_time_point(const std::time_t& t);

	MH_STUFF_API std::tm current_tm(time_zone zone);
	MH_STUFF_API std::time_t current_time_t();
	MH_STUFF_API std::chrono::system_clock::time_point current_time_point();
}

#ifndef MH_COMPILE_LIBRARY
#include "chrono_helpers.inl"
#endif
