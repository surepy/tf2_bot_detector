#pragma once

#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#undef min
#undef max

namespace mh
{
	template<size_t N, typename CharT = char, typename Traits = std::char_traits<CharT>>
	class base_format_string
	{
	public:
		static_assert(N >= 1);

		using this_type = base_format_string;
		using value_type = CharT;
		using traits_type = Traits;
		using array_type = std::array<value_type, N>;
		using view_type = std::basic_string_view<value_type, traits_type>;

		constexpr base_format_string() noexcept
		{
			m_String[0] = value_type(0);
		}
		constexpr this_type& operator=(const this_type&) noexcept = default;

		constexpr size_t size() const { return m_Length; }
		static constexpr size_t max_size() { return N - 1; }

		size_t vsprintf(const value_type* fmtStr, va_list args)
		{
			assert(m_Length <= max_size());
			const size_t maxWriteCount = (max_size() + 1) - m_Length; // max_size() + 1 because max_size() does not include null term

			// vsnprintf writes at must maxWriteCount - 1 chars, always adds null terminator
			const size_t writeCount = std::vsnprintf(m_String.data() + m_Length, maxWriteCount, fmtStr, args);

			assert(maxWriteCount >= 1);
			m_Length += std::min(maxWriteCount - 1, writeCount);
			assert(m_Length <= max_size());

			return m_Length;
		}
		size_t sprintf(const value_type* fmtStr, ...)
		{
			va_list args;
			va_start(args, fmtStr);
			const auto retVal = this->vsprintf(fmtStr, args);
			va_end(args);
			return retVal;
		}
		constexpr size_t puts(const view_type& str)
		{
			assert(m_Length <= max_size());
			const auto copyCount = std::min(str.size(), max_size() - m_Length);
#if __cpp_lib_is_constant_evaluated >= 201811
			if (!std::is_constant_evaluated())
			{
				std::memcpy(m_String.data() + m_Length, str.data(), copyCount * sizeof(value_type));
			}
			else
#endif
			{
				for (size_t i = 0; i < copyCount; i++)
					m_String[m_Length + i] = str[i];
			}

			m_Length += copyCount;
			m_String[m_Length] = 0;
			return m_Length;
		}

		template<typename... TArgs>
		auto fmt(const view_type& fmtStr, const TArgs&... args) ->
			decltype(fmt::format_to_n((CharT*)nullptr, max_size(), fmtStr, args...), *this)
		{
			const auto result = fmt::format_to_n(m_String.data() + m_Length, max_size() - m_Length, fmt::runtime(fmtStr), args...);
			m_Length = result.out - m_String.data();
			assert(m_Length <= max_size());
			m_String[m_Length] = value_type(0);
			return *this;
		}

		constexpr this_type& clear()
		{
			m_Length = 0;
			m_String[0] = 0;
			return *this;
		}

		constexpr this_type& operator=(const view_type& rhs)
		{
			puts(rhs);
			return *this;
		}

		[[nodiscard]] constexpr bool empty() const { return m_Length == 0; }

		constexpr const value_type* c_str() const { return m_String.data(); }
		constexpr const array_type& array() const { return m_String; }
		constexpr view_type view() const { return view_type(m_String.data(), m_Length); }

		template<typename TAlloc = std::allocator<value_type>>
		std::basic_string<value_type, traits_type, TAlloc> str() const
		{
			return std::basic_string<value_type, traits_type, TAlloc>(view());
		}

		constexpr operator view_type() const { return view(); }

	protected:
		size_t m_Length = 0;  // not including null terminator
		array_type m_String;
	};

	template<size_t N, typename CharT = char, typename Traits = std::char_traits<CharT>>
	class printf_string : public base_format_string<N, CharT, Traits>
	{
	public:
		using base_type = base_format_string<N, CharT, Traits>;
		using this_type = printf_string;
		using value_type = typename base_type::value_type;
		using traits_type = typename base_type::traits_type;
		using view_type = typename base_type::view_type;

		constexpr printf_string() = default;
		explicit printf_string(const value_type* fmtStr, ...)
		{
			va_list args;
			va_start(args, fmtStr);
			base_type::vsprintf(fmtStr, args);
			va_end(args);
		}

		constexpr this_type& operator=(const view_type& rhs)
		{
			base_type::puts(rhs);
			return *this;
		}
	};
	template<size_t N, typename CharT = char, typename Traits = std::char_traits<CharT>>
	using pfstr = printf_string<N, CharT, Traits>;

	template<size_t N, typename CharT = char, typename Traits = std::char_traits<CharT>>
	class format_string : public base_format_string<N, CharT, Traits>
	{
	public:
		using base_type = base_format_string<N, CharT, Traits>;
		using this_type = format_string;
		using value_type = typename base_type::value_type;
		using traits_type = typename base_type::traits_type;
		using view_type = typename base_type::view_type;
		using array_type = typename base_type::array_type;

		constexpr format_string() = default;
		template<typename... TArgs, typename = decltype(fmt::format(fmt::runtime(std::declval<view_type>()), std::declval<TArgs>()...))>
		explicit format_string(const view_type& fmtStr, TArgs&&... args)
		{
			base_type::fmt(fmtStr, std::forward<TArgs>(args)...);
		}

		constexpr this_type& operator=(const view_type& rhs)
		{
			base_type::puts(rhs);
			return *this;
		}
	};
	template<size_t N, typename CharT = char, typename Traits = std::char_traits<CharT>>
	using fmtstr = format_string<N, CharT, Traits>;
}

template<typename CharT, typename Traits, size_t N>
inline std::basic_ostream<CharT, Traits>& operator<<(
	std::basic_ostream<CharT, Traits>& os, const mh::base_format_string<N, CharT, Traits>& str)
{
	return os << str.view();
}

// fmt 10 needs an explicit formatter; format the fixed-buffer string as its view.
template<size_t N, typename CharT, typename Traits>
struct fmt::formatter<mh::format_string<N, CharT, Traits>, CharT>
	: fmt::formatter<std::basic_string_view<CharT, Traits>, CharT>
{
	template<typename FormatContext>
	auto format(const mh::format_string<N, CharT, Traits>& str, FormatContext& ctx) const
	{
		return fmt::formatter<std::basic_string_view<CharT, Traits>, CharT>::format(str.view(), ctx);
	}
};
template<size_t N, typename CharT, typename Traits>
struct fmt::formatter<mh::printf_string<N, CharT, Traits>, CharT>
	: fmt::formatter<std::basic_string_view<CharT, Traits>, CharT>
{
	template<typename FormatContext>
	auto format(const mh::printf_string<N, CharT, Traits>& str, FormatContext& ctx) const
	{
		return fmt::formatter<std::basic_string_view<CharT, Traits>, CharT>::format(str.view(), ctx);
	}
};
