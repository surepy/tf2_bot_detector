#pragma once

#include <type_traits>

namespace mh
{
	namespace detail::enum_class_bit_ops_hpp
	{
		template<typename T> struct bit_ops_enabled : std::false_type {};

		template<typename T>
		struct result
		{
			constexpr result(T value) : m_Value(value) {}

			constexpr operator bool() const { return m_Value != std::remove_reference_t<T>{}; }
			constexpr operator T() const { return m_Value; }

			T m_Value;
		};
	}
}

#define MH_ENABLE_ENUM_CLASS_BIT_OPS(type) \
	inline constexpr ::mh::detail::enum_class_bit_ops_hpp::result<type> operator&(type lhs, type rhs) \
	{ \
		using ut = std::underlying_type_t<type>; \
		return static_cast<type>(ut(lhs) & ut(rhs)); \
	} \
	inline constexpr ::mh::detail::enum_class_bit_ops_hpp::result<type&> operator&=(type& lhs, type rhs) \
	{ \
		using ut = std::underlying_type_t<type>; \
		return lhs = type(ut(lhs) & ut(rhs)); \
	} \
	inline constexpr ::mh::detail::enum_class_bit_ops_hpp::result<type> operator|(type lhs, type rhs) \
	{ \
		using ut = std::underlying_type_t<type>; \
		return static_cast<type>(ut(lhs) | ut(rhs)); \
	} \
	inline constexpr ::mh::detail::enum_class_bit_ops_hpp::result<type&> operator|=(type& lhs, type rhs) \
	{ \
		using ut = std::underlying_type_t<type>; \
		return lhs = type(ut(lhs) | ut(rhs)); \
	} 
