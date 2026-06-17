#pragma once

#include <stdexcept>
#include <type_traits>
#include <variant>

namespace mh
{
	template<typename TFunc>
	class lazy final
	{
	public:
		using func_type = TFunc;
		using value_type = std::invoke_result_t<TFunc>;

		constexpr lazy() = default;
		constexpr lazy(TFunc&& func) : m_Value(std::move(func)) {}

		constexpr operator const value_type& () const { return get(); }
		constexpr const value_type& operator()() const { return get(); }

		constexpr const value_type& get() const
		{
			if (m_Value.index() == 0)
				throw std::logic_error("Empty mh::lazy");

			if (func_type* func = std::get_if<1>(&m_Value))
				m_Value.emplace<2>(std::move((*func)()));

			return std::get<2>(m_Value);
		}

	private:
		mutable std::variant<std::monostate, func_type, value_type> m_Value;
	};
}
