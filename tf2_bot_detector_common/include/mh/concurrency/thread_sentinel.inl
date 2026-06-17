#ifdef MH_COMPILE_LIBRARY
#include "thread_sentinel.hpp"
#endif

#ifndef MH_COMPILE_LIBRARY_INLINE
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

#include <string>
#include <sstream>

namespace mh
{
	namespace detail::thread_sentinel_hpp
	{
		MH_COMPILE_LIBRARY_INLINE std::string make_exception_message(
			const mh::source_location& location, const std::thread::id& expectedID)
		{
			const auto currentID = std::this_thread::get_id();

			std::ostringstream ss;

			ss << "mh::thread_sentinel expected thread id " << expectedID
				<< ", but was triggered from thread " << currentID << " in " << location;

			return ss.str();
		}
	}

	MH_COMPILE_LIBRARY_INLINE thread_sentinel_exception::thread_sentinel_exception(
		const mh::source_location& location, const std::thread::id& expectedID) :
		std::runtime_error(detail::thread_sentinel_hpp::make_exception_message(location, expectedID)),
		m_Location(location)
	{
	}

	MH_COMPILE_LIBRARY_INLINE thread_sentinel::thread_sentinel() noexcept :
		m_ExpectedID(std::this_thread::get_id())
	{
	}

	MH_COMPILE_LIBRARY_INLINE void thread_sentinel::check(const mh::source_location& location) const
	{
		if (m_ExpectedID != std::this_thread::get_id())
			throw thread_sentinel_exception(location, m_ExpectedID);
	}

	MH_COMPILE_LIBRARY_INLINE void thread_sentinel::reset_id() noexcept
	{
		reset_id(std::this_thread::get_id());
	}

	MH_COMPILE_LIBRARY_INLINE void thread_sentinel::reset_id(const std::thread::id& id) noexcept
	{
		m_ExpectedID = id;
	}
}
