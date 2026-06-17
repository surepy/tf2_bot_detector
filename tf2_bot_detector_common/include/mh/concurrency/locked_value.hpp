#pragma once

#include <cassert>
#include <mutex>

namespace mh
{
	template<typename T, typename TMutex = std::mutex>
	class locked_value final
	{
	public:
		//using self_type = locked_value<T, TMutex>;
		using value_type = T;
		using mutex_type = TMutex;
		using lock_type = std::unique_lock<mutex_type>;

		locked_value() = default;
		locked_value(const T& initialVal) : m_Value(initialVal) {}
		locked_value(T&& initialVal) : m_Value(std::move(initialVal)) {}

		operator T() const { return get(); }
		locked_value& operator=(const T& value)
		{
			set(value);
			return *this;
		}
		locked_value& operator=(T&& value)
		{
			set(std::move(value));
			return *this;
		}

		T get() const
		{
			std::lock_guard lock(m_Mutex);
			return m_Value;
		}
		[[nodiscard]] lock_type lock() const { return lock_type(m_Mutex); }
		const T& get_ref([[maybe_unused]] const lock_type& lock) const
		{
			assert(lock.mutex() == &m_Mutex);
			assert(lock.owns_lock());
			return m_Value;
		}
		T& get_ref(const lock_type& lock)
		{
			return const_cast<T&>(const_cast<const locked_value*>(this)->get_ref(lock));
		}

		void set(const T& value)
		{
			std::lock_guard lock(m_Mutex);
			m_Value = value;
		}
		void set(T&& value)
		{
			std::lock_guard lock(m_Mutex);
			m_Value = std::move(value);
		}

	private:
		mutable mutex_type m_Mutex;
		T m_Value;
	};
}
