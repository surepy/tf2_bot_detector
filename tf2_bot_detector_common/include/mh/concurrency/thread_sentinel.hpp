#pragma once

#include <mh/source_location.hpp>

#include <stdexcept>
#include <thread>

namespace mh
{
	class thread_sentinel_exception : public std::runtime_error
	{
	public:
		MH_STUFF_API thread_sentinel_exception(const mh::source_location& location, const std::thread::id& expectedID);

		MH_STUFF_API const mh::source_location& location() const;

	private:
		mh::source_location m_Location;
	};

	class thread_sentinel
	{
	public:
		MH_STUFF_API thread_sentinel() noexcept;

		MH_STUFF_API void check(MH_SOURCE_LOCATION_AUTO(location)) const;

		MH_STUFF_API void reset_id() noexcept;
		MH_STUFF_API void reset_id(const std::thread::id& id) noexcept;

	private:
		std::thread::id m_ExpectedID;
	};
}

#ifndef MH_COMPILE_LIBRARY
#include "thread_sentinel.inl"
#endif
