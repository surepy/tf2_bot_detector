#pragma once

#include <chrono>
#include <mutex>

namespace mh
{
	namespace detail::cached_variable_hpp
	{
		using default_clock = std::chrono::steady_clock;

		template<typename T, typename TUpdateFunc, typename TClock>
		class cached_variable_base
		{
		public:
			using value_type = T;
			using clock_type = TClock;
			using duration_type = typename clock_type::duration;
			using time_point_type = typename clock_type::time_point;

			template<typename... TArgs>
			cached_variable_base(duration_type updateInterval, TUpdateFunc updateFunc, TArgs&&... updateFuncArgs) :
				m_NextUpdate(clock_type::now() + updateInterval),
				m_UpdateInterval(updateInterval),
				m_UpdateFunc(std::move(updateFunc)),
				m_Value(m_UpdateFunc(std::forward<TArgs>(updateFuncArgs)...))
			{
			}

			duration_type time_since_update() const
			{
				return clock_type::now() - (m_NextUpdate - m_UpdateInterval);
			}
			duration_type time_until_update() const
			{
				return m_NextUpdate - clock_type::now();
			}

		private:
			time_point_type m_NextUpdate{};
			duration_type m_UpdateInterval{};

#if __has_cpp_attribute(no_unique_address)
			[[no_unique_address]]
#endif
			TUpdateFunc m_UpdateFunc{};

		protected:
			template<typename... TArgs>
			bool try_update(TArgs&&... args)
			{
				auto now = clock_type::now();
				if (now >= m_NextUpdate)
				{
					m_Value = m_UpdateFunc(std::forward<TArgs>(args)...);
					m_NextUpdate = now + m_UpdateInterval;
					return true;
				}

				return false;
			}

			T m_Value{};
		};
	}

	template<bool threadSafe = true, typename T = void, typename TUpdateFunc = void,
		typename TClock = detail::cached_variable_hpp::default_clock>
	class cached_variable final : public detail::cached_variable_hpp::cached_variable_base<T, TUpdateFunc, TClock>
	{
		// Non-thread-safe version
		using cached_variable_base = detail::cached_variable_hpp::cached_variable_base<T, TUpdateFunc, TClock>;
	public:
		using cached_variable_base::cached_variable_base;

		T& get_no_update() { return cached_variable_base::m_Value; }
		const T& get_no_update() const { return cached_variable_base::m_Value; }

		template<typename... TArgs>
		T& get(TArgs&&... args)
		{
			cached_variable_base::try_update(std::forward<TArgs>(args)...);
			return cached_variable_base::m_Value;
		}
	};

	template<typename T, typename TUpdateFunc, typename TClock>
	class cached_variable<true, T, TUpdateFunc, TClock> final :
		public detail::cached_variable_hpp::cached_variable_base<T, TUpdateFunc, TClock>
	{
		// Thread-safe version
		using cached_variable_base = detail::cached_variable_hpp::cached_variable_base<T, TUpdateFunc, TClock>;
	public:
		using cached_variable_base::cached_variable_base;

		T get_no_update() const
		{
			std::lock_guard lock(m_Mutex);
			return cached_variable_base::m_Value;
		}

		template<typename... TArgs>
		T get(TArgs&&... args)
		{
			std::lock_guard lock(m_Mutex);
			cached_variable_base::try_update(std::forward<TArgs>(args)...);
			return cached_variable_base::m_Value;
		}

	private:
		mutable std::mutex m_Mutex;
	};

	template<bool threadSafe = true, typename TUpdateFunc = void, typename TClock = detail::cached_variable_hpp::default_clock, typename... TArgs>
	cached_variable(typename TClock::duration d, TUpdateFunc f, TArgs&&... args) -> cached_variable<threadSafe, std::invoke_result_t<TUpdateFunc, TArgs...>, TUpdateFunc, TClock>;
}
