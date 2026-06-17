#pragma once

#include <variant>

namespace mh
{
	// For when you want to pass a pointer to a function as an optional output parameter,
	// but simplifies the handling of logic inside that function so you don't have to care
	// if the pointer is null or not.
	template<typename T>
	class optional_ref final
	{
		using this_type = optional_ref<T>;

	public:
		using value_type = T;

		constexpr optional_ref() = default;
		constexpr optional_ref(T& value) : m_Value(&value) {}

		optional_ref(const this_type&) = delete;
		optional_ref(this_type&&) = delete;

		this_type& operator=(const this_type&) = delete;
		this_type& operator=(this_type&&) = delete;

		this_type& operator=(T&& value)
		{
			get() = std::move(value);
			return *this;
		}
		this_type& operator=(const T& value)
		{
			get() = value;
			return *this;
		}

		operator T& () { return get(); }
		operator const T& () const { return get(); }

		value_type& get() { return const_cast<value_type&>(const_cast<const this_type*>(this)->get()); }
		const value_type& get() const
		{
			if (m_Value.index() == 0)
				return std::get<0>(m_Value);
			else
				return *std::get<1>(m_Value);
		}

	private:
		std::variant<T, T*> m_Value;
	};
}
