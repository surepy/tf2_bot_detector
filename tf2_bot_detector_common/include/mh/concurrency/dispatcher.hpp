#pragma once

#if __has_include(<mh/coroutine/coroutine_include.hpp>)
#include <mh/coroutine/coroutine_include.hpp>
#endif

#ifdef MH_COROUTINES_SUPPORTED

#include <chrono>
#include <memory>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh
{
	namespace detail::dispatcher_hpp
	{
		struct thread_data;
		using clock_t = std::chrono::steady_clock;

		struct [[nodiscard]] co_dispatch_task
		{
			co_dispatch_task(std::shared_ptr<thread_data> threadData) noexcept;

			MH_STUFF_API bool await_ready() const;
			MH_STUFF_API void await_resume() const;
			MH_STUFF_API bool await_suspend(coro::coroutine_handle<> parent);

		private:
			std::shared_ptr<thread_data> m_ThreadData;
		};

		struct [[nodiscard]] co_delay_task
		{
			co_delay_task(std::shared_ptr<thread_data> threadData, clock_t::time_point delayUntilTime) noexcept;

			MH_STUFF_API bool await_ready() const;
			MH_STUFF_API void await_resume() const;
			MH_STUFF_API bool await_suspend(coro::coroutine_handle<> parent);

		private:
			std::shared_ptr<thread_data> m_ThreadData;
			clock_t::time_point m_DelayUntilTime;
		};
	}

	class dispatcher
	{
		using thread_data = detail::dispatcher_hpp::thread_data;

	public:
		using dispatch_task_t = detail::dispatcher_hpp::co_dispatch_task;
		using delay_task_t = detail::dispatcher_hpp::co_delay_task;
		using clock_t = detail::dispatcher_hpp::clock_t;

		MH_STUFF_API dispatcher(bool singleThread = true);

		MH_STUFF_API size_t task_count() const;

		MH_STUFF_API dispatch_task_t co_dispatch();
		MH_STUFF_API delay_task_t co_delay_for(clock_t::duration duration);
		MH_STUFF_API delay_task_t co_delay_until(clock_t::time_point endTime);

		template<typename TRep, typename TPeriod>
		delay_task_t co_delay_for(std::chrono::duration<TRep, TPeriod> duration)
		{
			return co_delay_until(clock_t::now() + std::chrono::duration_cast<clock_t::duration>(duration));
		}

		MH_STUFF_API size_t run();
		MH_STUFF_API bool run_one();
		template<typename TFunc>
		size_t run_while(TFunc&& func)
		{
			size_t count = 0;

			while (func() && run_one())
				count++;

			return count;
		}
		template<typename TClock, typename TDuration>
		size_t run_until(std::chrono::time_point<TClock, TDuration> endTime)
		{
			return run_while([&]()
				{
					auto now = TClock::now();
					return now < endTime;
				});
		}
		template<typename TRep, typename TPeriod>
		size_t run_for(std::chrono::duration<TRep, TPeriod> duration)
		{
			return run_until(clock_t::now() + duration);
		}

		MH_STUFF_API void wait_tasks() const;
		MH_STUFF_API bool wait_tasks_while(bool(*predicateFunc)(void* userData), void* userData = nullptr) const;
		MH_STUFF_API bool wait_tasks_while(bool(*predicateFunc)(const void* userData), const void* userData = nullptr) const;
		MH_STUFF_API bool wait_tasks_until(clock_t::time_point endTime) const;
		MH_STUFF_API bool wait_tasks_for(clock_t::duration duration) const;

		template<typename TFunc>
		bool wait_tasks_while(TFunc&& func) const
		{
			using vp_func = std::conditional_t<std::is_const_v<TFunc>, const void*, void*>;

			bool (*predicateFunc)(vp_func* userData) = [](vp_func* userData)
			{
				return (*reinterpret_cast<TFunc*>(userData))();
			};
			return wait_tasks_while(predicateFunc, reinterpret_cast<vp_func>(&func));
		}

	private:
		std::shared_ptr<thread_data> m_ThreadData;
	};
}

#ifndef MH_COMPILE_LIBRARY
#include "dispatcher.inl"
#endif

#endif
