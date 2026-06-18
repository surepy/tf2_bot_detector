#pragma once

#include <utility>

namespace mh
{
	template<typename T>
	class variable_pusher
	{
	public:
		constexpr variable_pusher(T& variable, const T& newValue) :
			m_Variable(variable),
			m_OldValue(std::move(variable))
		{
			m_Variable = newValue;
		}
		constexpr variable_pusher(T& variable, T&& newValue) :
			m_Variable(variable),
			m_OldValue(std::move(variable))
		{
			m_Variable = std::move(newValue);
		}
		~variable_pusher()
		{
			m_Variable = std::move(m_OldValue);
		}

		variable_pusher(const variable_pusher&) = delete;
		variable_pusher(variable_pusher&&) = delete;
		variable_pusher& operator=(const variable_pusher&) = delete;
		variable_pusher& operator=(const variable_pusher&&) = delete;

		constexpr const T& old_value() const { return m_OldValue; }

	private:
		T& m_Variable;
		T m_OldValue;
	};
}
