#ifdef MH_COMPILE_LIBRARY
#include "dispatcher.hpp"
#else
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

#ifdef MH_COROUTINES_SUPPORTED

#include <mh/error/not_implemented_error.hpp>
#include <mh/containers/heap.hpp>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#undef min
#undef max

namespace mh
{
	namespace detail::dispatcher_hpp
	{
		struct task_data
		{
			mutable std::mutex m_TaskCompleteCVMutex;
			mutable std::condition_variable m_TaskCompleteCV;
			std::atomic<bool> m_IsTaskComplete = false;

			coro::coroutine_handle<> m_Handle;
		};

		struct task_delay_data
		{
			constexpr bool operator<(const task_delay_data& rhs) const
			{
				// intentionally reversed
				return rhs.m_DelayUntilTime < m_DelayUntilTime;
			}

			clock_t::time_point m_DelayUntilTime;
			coro::coroutine_handle<> m_Handle;
		};

		struct thread_data
		{
			thread_data(bool singleThread) :
				m_IsSingleThread(singleThread)
			{
			}

			coro::coroutine_handle<> try_pop_task()
			{
				if (!m_Tasks.empty() || !m_DelayTasks.empty())
				{
					std::lock_guard lock(m_TasksMutex);

					if (!m_DelayTasks.empty())
					{
						auto now = clock_t::now();
						const task_delay_data& taskDelayData = m_DelayTasks.front();
						if (taskDelayData.m_DelayUntilTime <= now)
						{
							auto task = taskDelayData.m_Handle;
							m_DelayTasks.pop();
							return task;
						}
					}

					if (!m_Tasks.empty())
					{
						auto task = m_Tasks.front();
						m_Tasks.pop();
						return task;
					}
				}

				return nullptr;
			}

			void add_task(coro::coroutine_handle<> task)
			{
				std::lock_guard lock(m_TasksMutex);
				m_Tasks.push(task);
				m_TasksAvailableCV.notify_one();
			}
			void add_delay_task(task_delay_data data)
			{
				std::lock_guard lock(m_TasksMutex);
				m_DelayTasks.push(std::move(data));
			}

			bool wait_tasks_until(const clock_t::time_point endTime) const
			{
				std::unique_lock lock(m_TasksMutex);

				const auto IsTaskAvailable = [&]
				{
					if (!m_DelayTasks.empty() && m_DelayTasks.front().m_DelayUntilTime <= clock_t::now())
						return true;

					if (!m_Tasks.empty())
						return true;

					return false;
				};

				while (endTime > clock_t::now())
				{
					auto localEndTime = endTime;
					if (!m_DelayTasks.empty())
						localEndTime = std::min(localEndTime, m_DelayTasks.front().m_DelayUntilTime);

					if (m_TasksAvailableCV.wait_until(lock, localEndTime, IsTaskAvailable))
						return true;
				}

				return false;
			}

			size_t task_count() const { return m_Tasks.size() + m_DelayTasks.size(); }

			bool m_IsSingleThread{};

			const std::thread::id m_OwnerThread = std::this_thread::get_id();

		private:
			mutable std::mutex m_TasksMutex;
			mutable std::condition_variable m_TasksAvailableCV;
			std::queue<coro::coroutine_handle<>> m_Tasks;
			mh::heap<task_delay_data> m_DelayTasks;
		};

		MH_COMPILE_LIBRARY_INLINE co_dispatch_task::co_dispatch_task(std::shared_ptr<thread_data> threadData) noexcept :
			m_ThreadData(std::move(threadData))
		{
		}

		MH_COMPILE_LIBRARY_INLINE bool co_dispatch_task::await_ready() const
		{
			return m_ThreadData->m_IsSingleThread && std::this_thread::get_id() == m_ThreadData->m_OwnerThread;
		}

		MH_COMPILE_LIBRARY_INLINE void co_dispatch_task::await_resume() const
		{
			assert(!m_ThreadData->m_IsSingleThread || m_ThreadData->m_OwnerThread == std::this_thread::get_id());
			//assert(m_TaskData);
			//std::unique_lock lock(m_TaskData->m_TaskCompleteCVMutex);
			//m_TaskData->m_TaskCompleteCV.wait(lock, [&] { return !m_TaskData->m_IsTaskComplete; });
		}

		MH_COMPILE_LIBRARY_INLINE bool co_dispatch_task::await_suspend(coro::coroutine_handle<> handle)
		{
			// Should never hit this, await_ready() should prevent suspension of coroutines
			// that are already on the correct thread (unless we are not single threaded, in which case we *want*
			// to be able to defer
			assert(!m_ThreadData->m_IsSingleThread || std::this_thread::get_id() != m_ThreadData->m_OwnerThread);

			m_ThreadData->add_task(handle);

			return true;  // always suspend
		}

		MH_COMPILE_LIBRARY_INLINE co_delay_task::co_delay_task(
			std::shared_ptr<thread_data> threadData, clock_t::time_point delayUntilTime) noexcept :
			m_ThreadData(std::move(threadData)), m_DelayUntilTime(std::move(delayUntilTime))
		{
		}

		MH_COMPILE_LIBRARY_INLINE bool co_delay_task::await_ready() const
		{
			return m_DelayUntilTime <= clock_t::now();
		}
		MH_COMPILE_LIBRARY_INLINE void co_delay_task::await_resume() const
		{
			//throw mh::not_implemented_error();
		}
		MH_COMPILE_LIBRARY_INLINE bool co_delay_task::await_suspend(coro::coroutine_handle<> parent)
		{
			if (await_ready())
				return false; // no need for suspension

			{
				task_delay_data data;
				data.m_DelayUntilTime = m_DelayUntilTime;
				data.m_Handle = parent;
				m_ThreadData->add_delay_task(std::move(data));
			}

			return true; // suspend
		}
	}

	MH_COMPILE_LIBRARY_INLINE dispatcher::dispatcher(bool singleThread) :
		m_ThreadData(std::make_shared<thread_data>(singleThread))
	{
	}

	MH_COMPILE_LIBRARY_INLINE size_t dispatcher::run()
	{
		size_t count = 0;

		while (run_one())
			count++;

		return count;
	}

	MH_COMPILE_LIBRARY_INLINE bool dispatcher::run_one()
	{
		{
			const bool isAllowed = !m_ThreadData->m_IsSingleThread || m_ThreadData->m_OwnerThread == std::this_thread::get_id();
			assert(isAllowed);
			if (!isAllowed)
				return false;
		}

		using detail::dispatcher_hpp::task_data;

		if (mh::detail::coro::coroutine_handle<> task = m_ThreadData->try_pop_task())
		{
			// This could throw (...can it? what about promise_type::unhandled_exception()?)
			task.resume();
			return true;
		}

		return false;
	}

	MH_COMPILE_LIBRARY_INLINE detail::dispatcher_hpp::co_dispatch_task dispatcher::co_dispatch()
	{
		assert(m_ThreadData);
		return { m_ThreadData };
	}

	MH_COMPILE_LIBRARY_INLINE size_t dispatcher::task_count() const
	{
		return m_ThreadData->task_count();
	}

	MH_COMPILE_LIBRARY_INLINE bool dispatcher::wait_tasks_for(clock_t::duration duration) const
	{
		return wait_tasks_until(clock_t::now() + duration);
	}
	MH_COMPILE_LIBRARY_INLINE bool dispatcher::wait_tasks_until(clock_t::time_point endTime) const
	{
		return m_ThreadData->wait_tasks_until(endTime);
	}

	MH_COMPILE_LIBRARY_INLINE detail::dispatcher_hpp::co_delay_task dispatcher::co_delay_for(clock_t::duration duration)
	{
		return co_delay_until(clock_t::now() + duration);
	}
	MH_COMPILE_LIBRARY_INLINE detail::dispatcher_hpp::co_delay_task dispatcher::co_delay_until(clock_t::time_point endTime)
	{
		return detail::dispatcher_hpp::co_delay_task(m_ThreadData, endTime);
	}
}

#endif
