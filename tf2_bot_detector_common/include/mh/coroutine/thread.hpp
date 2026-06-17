#pragma once

#include "coroutine_include.hpp"

#ifdef MH_COROUTINES_SUPPORTED

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh
{
	namespace detail::coroutine::thread_hpp
	{
		enum class co_create_thread_flags
		{
			none = 0,

			// Only create move execution to another thread if the current thread is the main thread
			off_main_thread = (1 << 0),
		};

		struct task;

		struct [[nodiscard]] promise
		{
			//task get_return_object();

			constexpr coro::suspend_never initial_suspend() const noexcept { return {}; }
			constexpr coro::suspend_always final_suspend() const noexcept { return {}; }
		};

		struct [[nodiscard]] task
		{
			using promise_type = promise;

			task(co_create_thread_flags flags = co_create_thread_flags::none);

			constexpr bool await_ready() const { return false; }
			constexpr void await_resume() const {}
			MH_STUFF_API bool await_suspend(coro::coroutine_handle<> parent);

		private:
			co_create_thread_flags m_Flags;
		};
	}

	// Moves execution to a new thread
	MH_STUFF_API detail::coroutine::thread_hpp::task co_create_thread();

	// Only moves execution to a new thread if we're currently on the main thread
	MH_STUFF_API detail::coroutine::thread_hpp::task co_create_background_thread();
}

#ifndef MH_COMPILE_LIBRARY
#include <mh/coroutine/thread.inl>
#endif

#endif
