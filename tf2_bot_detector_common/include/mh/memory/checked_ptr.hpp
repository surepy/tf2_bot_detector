#pragma once

#if __has_include(<compare>)
#include <compare>
#endif

#ifdef _DEBUG
#include <cassert>
#endif

namespace mh
{
	template<typename T>
	class checked_ptr final
	{
	public:
		using this_type = checked_ptr<T>;
		using value_type = T*;

		constexpr checked_ptr() = default;
		constexpr checked_ptr(value_type ptr) : m_Ptr(ptr) {}

		constexpr bool has_value() const
		{
#ifdef _DEBUG
			m_Checked = true;
#endif
			return !!m_Ptr;
		}
		constexpr operator bool() const { return has_value(); }

		constexpr value_type get() const
		{
#ifdef _DEBUG
			assert(m_Checked);
#endif
			return m_Ptr;
		}
		constexpr operator value_type() const { return get(); }
		constexpr value_type operator->() const { return get(); }

	private:
		value_type m_Ptr = nullptr;

#ifdef _DEBUG
		mutable bool m_Checked = false;
#endif
	};

#if __has_include(<compare>) && (__cpp_impl_three_way_comparison >= 201907)
	template<typename T>
	inline constexpr std::strong_ordering operator<=>(const checked_ptr<T>& lhs, const checked_ptr<T>& rhs)
	{
		return lhs.m_Ptr <=> rhs.m_Ptr;
	}
	template<typename T>
	inline constexpr std::strong_ordering operator<=>(const T* lhs, const checked_ptr<T>& rhs)
	{
		return lhs <=> rhs.m_Ptr;
	}
	template<typename T>
	inline constexpr std::strong_ordering operator<=>(const checked_ptr<T>& lhs, const T* rhs)
	{
		return lhs.m_Ptr <=> rhs;
	}
#endif

	template<typename T>
	inline constexpr bool operator==(const checked_ptr<T>& lhs, const checked_ptr<T>& rhs)
	{
		return lhs.m_Ptr == rhs.m_Ptr;
	}
	template<typename T>
	inline constexpr bool operator==(const T* lhs, const checked_ptr<T>& rhs)
	{
		return lhs == rhs.m_Ptr;
	}
	template<typename T>
	inline constexpr bool operator==(const checked_ptr<T>& lhs, const T* rhs)
	{
		return lhs.m_Ptr == rhs;
	}
}
