#pragma once

#include "coroutine_include.hpp"

#ifdef MH_COROUTINES_SUPPORTED

#include "../concurrency/mutex_debug.hpp"
#include "../data/variable_pusher.hpp"
#include "../memory/stack_info.hpp"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <future>
#include <variant>
#include <vector>

namespace mh
{
	template<typename T = void> class task;

	enum class task_state
	{
		empty, // no state, never initialized, or was moved from
		running,
		value,
		exception,
	};

	namespace detail::task_hpp
	{
		template<typename T>
		struct co_promise_traits
		{
			using value_type = T;
			using reference = T&;
			using const_reference = const T&;
			using storage_type = T;
		};

		template<>
		struct co_promise_traits<void>
		{
			using value_type = void;
			using reference = void;
			using const_reference = void;
			using storage_type = std::monostate;
		};

		template<typename T>
		struct co_promise_traits<T&>
		{
			using value_type = T&;
			using reference = T&;
			using const_reference = const T&;
			using storage_type = std::reference_wrapper<std::remove_reference_t<T>>;
		};

		struct suspend_sometimes
		{
			constexpr suspend_sometimes(bool suspend) : m_Suspend(suspend) {}

			[[nodiscard]] constexpr bool await_ready() const noexcept { return !m_Suspend; }
			constexpr void await_suspend(coro::coroutine_handle<>) const noexcept {}
			constexpr void await_resume() const noexcept {}

			bool m_Suspend;
		};

		template<typename T>
		class promise_base
		{
			using traits = co_promise_traits<T>;
			using storage_type = typename traits::storage_type;

			static constexpr int32_t REFCOUNT_UNSET = -1234;

		public:
			~promise_base()
			{
				assert(m_RefCount == 0);
			}

			promise_base() noexcept {}
			promise_base(const promise_base<T>&) = delete;
			promise_base(promise_base<T>&&) = delete;
			promise_base<T>& operator=(const promise_base<T>&) = delete;
			promise_base<T>& operator=(promise_base<T>&&) = delete;

			static constexpr size_t IDX_WAITERS = 0;
			static constexpr size_t IDX_INVALID = 1;
			static constexpr size_t IDX_VALUE = 2;
			static constexpr size_t IDX_EXCEPTION = 3;

			constexpr task<T> get_return_object();

			bool is_ready() const noexcept
			{
				auto index = m_State.index();
				return index == IDX_VALUE || index == IDX_EXCEPTION;
			}

			bool valid() const noexcept
			{
				return m_State.index() != IDX_INVALID;
			}

			std::exception_ptr get_exception() const noexcept
			{
				auto result = std::get_if<IDX_EXCEPTION>(&m_State);
				return result ? *result : nullptr;
			}

			task_state get_task_state() const
			{
				const auto state = m_State.index();
				switch (state)
				{
				case IDX_INVALID:    return task_state::empty;
				default:
					assert(!"Invalid state in mh::detail::task_hpp::promise_base<T>::get_task_state()");
					[[fallthrough]];
				case IDX_WAITERS:    return task_state::running;
				case IDX_VALUE:      return task_state::value;
				case IDX_EXCEPTION:  return task_state::exception;
				}
			}

			void wait() const
			{
				if (!valid())
					throw std::future_error(std::future_errc::no_state);

				if (!is_ready())
				{
					std::unique_lock lock(m_Mutex);
					m_ValueReadyCV.wait(lock, [&] { return is_ready(); });
					assert(is_ready());
				}
			}
			template<typename Rep, typename Period>
			std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const
			{
				if (!valid())
					throw std::future_error(std::future_errc::no_state);

				std::unique_lock lock(m_Mutex);
				if (is_ready())
					return std::future_status::ready;

				if (!m_ValueReadyCV.wait_for(lock, timeout_duration, [&] { return is_ready(); }))
					return std::future_status::timeout;

				assert(is_ready());
				return std::future_status::ready;
			}
			template<typename Clock, typename Period>
			std::future_status wait_until(const std::chrono::time_point<Clock, Period>& timeout_time) const
			{
				if (!valid())
					throw std::future_error(std::future_errc::no_state);

				std::unique_lock lock(m_Mutex);
				if (is_ready())
					return std::future_status::ready;

				if (!m_ValueReadyCV.wait_until(lock, timeout_time, [&] { return is_ready(); }))
					return std::future_status::timeout;

				assert(is_ready());
				return std::future_status::ready;
			}

			void rethrow_if_exception() const
			{
				std::exception_ptr ex;
				{
					std::lock_guard lock(m_Mutex);
					if (auto e = std::get_if<IDX_EXCEPTION>(&m_State))
						ex = *e;
				}

				if (ex)
					std::rethrow_exception(ex);
			}

			T take_value()
			{
				wait();
				rethrow_if_exception();

				if constexpr (!std::is_void_v<T>)
				{
					auto value = std::move(std::get<IDX_VALUE>(m_State));
					m_State.template emplace<IDX_INVALID>();
					return std::move(value);
				}
			}

			void unhandled_exception()
			{
				set_state<IDX_EXCEPTION>(std::current_exception());
			}

			constexpr coro::suspend_never initial_suspend() const noexcept { return {}; }
			suspend_sometimes final_suspend() const noexcept
			{
				std::lock_guard lock(m_Mutex);
				m_FinalSuspendHasRun = true;

				// If m_RefCount == 0, we are in charge of our own destiny (all referencing tasks have gone out of
				// scope, so just delete ourselves when we're done)
				return m_RefCount != 0;
			}

			bool await_ready() const { return is_ready(); }
			bool await_suspend(coro::coroutine_handle<> parent)
			{
				if (is_ready())
				{
					return false;
				}
				else
				{
					std::lock_guard lock(m_Mutex);
					if (is_ready())
					{
						return false;
					}
					else
					{
						std::get<IDX_WAITERS>(m_State).push_back(parent);
						return true; // suspend
					}
				}
			}

			template<size_t IDX, typename TValue>
			void set_state(TValue&& value)
			{
				std::vector<coro::coroutine_handle<>> waiters;

				{
					std::lock_guard lock(m_Mutex);

					if (is_ready())
						throw std::future_error(std::future_errc::promise_already_satisfied);

					waiters = std::move(std::get<IDX_WAITERS>(m_State));

					static_assert(IDX == IDX_VALUE || IDX == IDX_EXCEPTION);
					m_State.template emplace<IDX>(std::move(value));

					m_ValueReadyCV.notify_all();
				}

				for (auto& waiter : waiters)
				{
					assert(m_RefCount > 0 || !final_suspend_has_run());
					waiter.resume();
					assert(m_RefCount > 0 || !final_suspend_has_run());
				}
			}
			
			const storage_type* try_get_value() const { return std::get_if<IDX_VALUE>(&m_State); }
			storage_type* try_get_value() { return std::get_if<IDX_VALUE>(&m_State); }

			void add_ref()
			{
				int32_t refCountUnset = REFCOUNT_UNSET;
				if (!m_RefCount.compare_exchange_strong(refCountUnset, 1))
				{
					assert(m_RefCount >= 1);
					++m_RefCount;
				}
			}
			[[nodiscard]] bool remove_ref()
			{
				auto newVal = --m_RefCount;
				assert(newVal >= 0);
				return newVal <= 0;
			}

			bool final_suspend_has_run() const noexcept { return m_FinalSuspendHasRun; }
			int32_t get_ref_count() const noexcept { return m_RefCount; }

			template<typename TFreeFunc>
			void release_promise_ref(TFreeFunc&& freeFunc)
			{
				std::unique_lock scopeLock(m_Mutex);

				if (remove_ref() && final_suspend_has_run())
				{
					scopeLock.unlock();
					freeFunc();
				}
			}

		protected:
			mutable std::condition_variable_any m_ValueReadyCV;
			mutable mh::mutex_debug<> m_Mutex;
			std::variant<std::vector<coro::coroutine_handle<>>, std::monostate, storage_type, std::exception_ptr> m_State;
			std::atomic_int32_t m_RefCount = REFCOUNT_UNSET;
			mutable bool m_FinalSuspendHasRun = false;
		};
	}

	namespace detail
	{
		template<typename T>
		class promise final : public task_hpp::promise_base<T>
		{
			using super = detail::task_hpp::promise_base<T>;

		public:
			decltype(auto) get_value() { return const_cast<T&>(std::as_const(*this).get_value()); }
			decltype(auto) get_value() const
			{
				this->wait();

				this->rethrow_if_exception();

				return std::get<super::IDX_VALUE>(this->m_State);
			}

			T* try_get_value() noexcept { return const_cast<T*>(std::as_const(*this).try_get_value()); }
			const T* try_get_value() const noexcept
			{
				std::lock_guard lock(this->m_Mutex);
				return std::get_if<super::IDX_VALUE>(std::addressof(this->m_State));
			}

			void return_value(T value)
			{
				this->template set_state<super::IDX_VALUE>(std::move(value));
			}

			T& await_resume() { return const_cast<T&>(std::as_const(*this).get_value()); }
			const T& await_resume() const
			{
				assert(this->is_ready());
				return this->get_value();
			}
		};

		template<>
		class promise<void> final : public task_hpp::promise_base<void>
		{
			using super = detail::task_hpp::promise_base<void>;

		public:
			void return_void()
			{
				this->set_state<super::IDX_VALUE>(std::monostate{});
			}

			void await_resume()
			{
				assert(this->is_ready());
				this->rethrow_if_exception();
			}
		};
	}

	namespace detail::task_hpp
	{
		template<typename T>
		class task_base
		{
		public:
			using promise_type = promise<T>;
			using coroutine_type = coro::coroutine_handle<promise_type>;

			constexpr task_base() noexcept : task_base(nullptr) {}
			explicit constexpr task_base(std::nullptr_t) noexcept : m_HandleOpt(nullptr), m_IsCoroutine(false) {}
			explicit task_base(coroutine_type state) noexcept :
				m_HandleOpt(std::move(state)),
				m_IsCoroutine(true)
			{
				if (promise_type* promise = try_get_promise())
					promise->add_ref();
			}
			explicit task_base(promise_type* promise) noexcept :
				m_PromiseOpt(promise),
				m_IsCoroutine(false)
			{
				if (promise_type* promise = try_get_promise())
					promise->add_ref();
			}

			task_base(const task_base& other) noexcept :
				m_IsCoroutine(other.m_IsCoroutine)
			{
				if (m_IsCoroutine)
					m_HandleOpt = other.m_HandleOpt;
				else
					m_PromiseOpt = other.m_PromiseOpt;

				if (promise_type* promise = try_get_promise())
					promise->add_ref();
			}
			task_base& operator=(const task_base& other) noexcept
			{
				release();

				m_IsCoroutine = other.m_IsCoroutine;
				if (m_IsCoroutine)
					m_HandleOpt = other.m_HandleOpt;
				else
					m_PromiseOpt = other.m_PromiseOpt;

				if (promise_type* promise = try_get_promise())
					promise->add_ref();

				return *this;
			}

			task_base(task_base&& other) noexcept :
				m_IsCoroutine(other.m_IsCoroutine)
			{
				assert(std::addressof(other) != this);
				if (m_IsCoroutine)
					m_HandleOpt = std::exchange(other.m_HandleOpt, nullptr);
				else
					m_PromiseOpt = std::exchange(other.m_PromiseOpt, nullptr);
			}
			task_base& operator=(task_base&& other) noexcept
			{
				assert(std::addressof(other) != this);
				release();

				m_IsCoroutine = other.m_IsCoroutine;
				if (m_IsCoroutine)
					m_HandleOpt = std::exchange(other.m_HandleOpt, nullptr);
				else
					m_PromiseOpt = std::exchange(other.m_PromiseOpt, nullptr);

				return *this;
			}

			~task_base()
			{
				release();
			}

			task_state state() const
			{
				const promise_type* promise = try_get_promise();
				return promise ? promise->get_task_state() : task_state::empty;
			}

			operator bool() const { return valid(); }
			[[nodiscard]] bool valid() const
			{
				const promise_type* promise = try_get_promise();
				return promise ? promise->valid() : false;
			}
			[[nodiscard]] bool is_ready() const
			{
				const promise_type* promise = try_get_promise();
				return promise ? promise->is_ready() : false;
			}
			[[nodiscard]] bool empty() const { return !try_get_promise(); }

			void wait() const
			{
				return get_promise().wait();
			}
			template<typename Rep, typename Period>
			std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const
			{
				return get_promise().wait_for(timeout_duration);
			}
			template<typename Clock, typename Period>
			std::future_status wait_until(const std::chrono::time_point<Clock, Period>& timeout_time) const
			{
				return get_promise().wait_until(timeout_time);
			}

			std::exception_ptr get_exception() const
			{
				const promise_type* promise = try_get_promise();
				return promise ? promise->get_exception() : nullptr;
			}

#if 0       // operator co_await makes intellisense very unhappy
			promise_type& operator co_await() { return get_promise(); }
			const promise_type& operator co_await() const { return get_promise(); }
#else
			bool await_ready() const { return get_promise().await_ready(); }
			bool await_suspend(coro::coroutine_handle<> parent) { return get_promise().await_suspend(parent); }
			decltype(auto) await_resume() { return get_promise().await_resume(); }
			decltype(auto) await_resume() const { return get_promise().await_resume(); }
#endif

		protected:
			promise_type* try_get_promise() { return const_cast<promise_type*>(std::as_const(*this).try_get_promise()); }
			const promise_type* try_get_promise() const
			{
				if (m_IsCoroutine)
					return m_HandleOpt ? &m_HandleOpt.promise() : nullptr;
				else
					return m_PromiseOpt;
			}
			promise_type& get_promise() { return const_cast<promise_type&>(std::as_const(*this).get_promise()); }
			const promise_type& get_promise() const
			{
				const promise_type* promise = try_get_promise();
				if (!promise)
					throw std::future_error(std::future_errc::no_state);

				return *promise;
			}

			promise_type* get_promise_for_copy() { return std::as_const(*this).get_promise_for_copy(); }
			promise_type* get_promise_for_copy() const
			{
				assert(!m_IsCoroutine);
				return m_IsCoroutine ? nullptr : m_PromiseOpt;
			}

			void release()
			{
				if (m_IsCoroutine)
				{
					if (m_HandleOpt)
					{
						m_HandleOpt.promise().release_promise_ref([&]
							{
								m_HandleOpt.destroy();
							});

						m_HandleOpt = nullptr;
					}
				}
				else if (m_PromiseOpt)
				{
					m_PromiseOpt->release_promise_ref([&]
						{
							delete m_PromiseOpt;
						});

					m_PromiseOpt = nullptr;
				}
			}

		private:
			union
			{
				coroutine_type m_HandleOpt;
				promise_type* m_PromiseOpt;
			};
			bool m_IsCoroutine;
		};
	}

	template<typename T>
	class task : public detail::task_hpp::task_base<T>
	{
		using super = detail::task_hpp::task_base<T>;

	public:
		using super::super;
		
		~task() {}

		const T& get() const { return this->get_promise().get_value(); }
		T& get() { return this->get_promise().get_value(); }

		const T* try_get() const
		{
			const auto promise = this->try_get_promise();
			return promise ? promise->try_get_value() : nullptr;
		}
		T* try_get() { return const_cast<T*>(std::as_const(*this).try_get()); }
	};

	template<>
	class task<void> : public detail::task_hpp::task_base<void>
	{
		using super = detail::task_hpp::task_base<void>;

	public:
		using super::super;
		~task() {}
	};

	template<typename T>
	inline constexpr task<T> detail::task_hpp::promise_base<T>::get_return_object()
	{
		return task<T>(coro::coroutine_handle<detail::promise<T>>::from_promise(*static_cast<promise<T>*>(this)));
	}

	template<typename T, typename... TArgs>
	inline task<T> make_ready_task(TArgs&&... args)
	{
		detail::promise<T>* promise = new detail::promise<T>();
		task<T> retVal(promise);

		promise->template set_state<detail::promise<T>::IDX_VALUE>(T(std::forward<TArgs>(args)...));

		return retVal;
	}
}

#endif
