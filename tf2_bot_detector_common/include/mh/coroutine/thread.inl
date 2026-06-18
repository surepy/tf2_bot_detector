#ifdef MH_COROUTINES_SUPPORTED

#ifdef MH_COMPILE_LIBRARY
#include <mh/coroutine/thread.hpp>
#endif

#ifndef MH_COMPILE_LIBRARY_INLINE
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

#if __has_include (<mh/concurrency/main_thread.hpp>)
#include <mh/concurrency/main_thread.hpp>
namespace mh::detail::coroutine::thread_hpp
{
	MH_COMPILE_LIBRARY_INLINE bool is_main_thread()
	{
		return mh::is_main_thread();
	}
}
#else
namespace mh::detail::coroutine::thread_hpp
{
	$MH_COMPILE_LIBRARY_INLINE static std::thread::id s_MainThreadID = std::this_thread::get_id();
	$MH_COMPILE_LIBRARY_INLINE bool is_main_thread()
	{
		return std::this_thread::get_id() == s_MainThreadID;
	}
}
#endif

#include <cassert>
#include <thread>

namespace mh::detail::coroutine::thread_hpp
{
	MH_COMPILE_LIBRARY_INLINE bool task::await_suspend(coro::coroutine_handle<> waiter)
	{
		if (m_Flags == co_create_thread_flags::none ||
			(m_Flags == co_create_thread_flags::off_main_thread && is_main_thread()))
		{
			std::thread t([](coro::coroutine_handle<> waiter) { waiter.resume(); }, waiter);
			t.detach();

			// always suspend
			return true;
		}
		else
		{
			// resume on current thread
			return false;
		}
	}

	//$MH_COMPILE_LIBRARY_INLINE task promise::get_return_object()
	//{
	//	return {};
	//}

	MH_COMPILE_LIBRARY_INLINE task::task(co_create_thread_flags flags) :
		m_Flags(flags)
	{
	}
}

MH_COMPILE_LIBRARY_INLINE mh::detail::coroutine::thread_hpp::task mh::co_create_thread()
{
	return {};
}

MH_COMPILE_LIBRARY_INLINE mh::detail::coroutine::thread_hpp::task mh::co_create_background_thread()
{
	return { detail::coroutine::thread_hpp::co_create_thread_flags::off_main_thread };
}

#endif
