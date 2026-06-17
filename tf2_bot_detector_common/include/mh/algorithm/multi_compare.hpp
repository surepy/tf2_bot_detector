#pragma once

#include <utility>

namespace mh
{
	// https://en.cppreference.com/w/cpp/language/fold
	template<typename TComparison, typename TValue, typename... TArgs>
	inline constexpr bool any_compare(TComparison&& comp, const TValue& lhs, const TArgs&... rhs)
	{
		return (... || (comp)(lhs, rhs));
	}
	template<typename TComparison, typename TValue, typename... TArgs>
	inline constexpr bool none_compare(TComparison&& comp, const TValue& lhs, const TArgs&... rhs)
	{
		return !any_compare(std::forward<TComparison>(comp), lhs, rhs...);
	}
	template<typename TComparison, typename TValue, typename... TArgs>
	inline constexpr bool all_compare(TComparison&& comp, const TValue& lhs, const TArgs&... rhs)
	{
		return (... && (comp)(lhs, rhs));
	}

#undef MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER
#define MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(name, comparer) \
	template<typename TValue, typename... TArgs> \
	inline constexpr bool any_ ## name(const TValue& lhs, const TArgs&... rhs) \
	{ \
		return any_compare(comparer, lhs, rhs...); \
	} \
	template<typename TValue, typename... TArgs> \
	inline constexpr bool none_ ## name(const TValue& lhs, const TArgs&... rhs) \
	{ \
		return none_compare(comparer, lhs, rhs...); \
	} \
	template<typename TValue, typename... TArgs> \
	inline constexpr bool all_ ## name(const TValue& lhs, const TArgs&... rhs) \
	{ \
		return all_compare(comparer, lhs, rhs...); \
	}

	MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(eq, [](const auto& lhs, const auto& rhs) { return lhs == rhs; })
	MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(neq, [](const auto& lhs, const auto& rhs) { return lhs != rhs; })
	MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(greater, [](const auto& lhs, const auto& rhs) { return lhs > rhs; })
	MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(greater_equal, [](const auto& lhs, const auto& rhs) { return lhs >= rhs; })
	MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(less, [](const auto& lhs, const auto& rhs) { return lhs < rhs; })
	MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER(less_equal, [](const auto& lhs, const auto& rhs) { return lhs <= rhs; })

#undef MH_ALGORITHM_MULTI_COMPARE_HPP_HELPER
}
