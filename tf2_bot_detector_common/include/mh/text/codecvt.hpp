#pragma once

#include <string>
#include <string_view>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

#ifndef MH_HAS_UNICODE
#define MH_HAS_UNICODE (__cpp_unicode_characters >= 200704)
#endif

#ifndef MH_HAS_CHAR8
#define MH_HAS_CHAR8 (__cpp_char8_t >= 201811)
#endif

#ifndef MH_HAS_CUCHAR
#define MH_HAS_CUCHAR (__has_include(<cuchar>))
#endif

namespace mh
{
	template<typename To, typename From, typename FromTraits = std::char_traits<From>>
	MH_STUFF_API std::basic_string<To> change_encoding(const std::basic_string_view<From, FromTraits>& input);

#ifdef MH_COMPILE_LIBRARY
	extern template MH_STUFF_API std::string change_encoding<char, char>(const std::string_view&);
	extern template MH_STUFF_API std::string change_encoding<char, wchar_t>(const std::wstring_view&);

	extern template MH_STUFF_API std::wstring change_encoding<wchar_t, char>(const std::string_view&);
	extern template MH_STUFF_API std::wstring change_encoding<wchar_t, wchar_t>(const std::wstring_view&);

#if MH_HAS_UNICODE
#if MH_HAS_CUCHAR
	extern template MH_STUFF_API std::u16string change_encoding<char16_t, char>(const std::string_view&);
	extern template MH_STUFF_API std::u16string change_encoding<char16_t, wchar_t>(const std::wstring_view&);
	extern template MH_STUFF_API std::u32string change_encoding<char32_t, char>(const std::string_view&);
	extern template MH_STUFF_API std::u32string change_encoding<char32_t, wchar_t>(const std::wstring_view&);

	extern template MH_STUFF_API std::string change_encoding<char, char16_t>(const std::u16string_view&);
	extern template MH_STUFF_API std::string change_encoding<char, char32_t>(const std::u32string_view&);
	extern template MH_STUFF_API std::wstring change_encoding<wchar_t, char16_t>(const std::u16string_view&);
	extern template MH_STUFF_API std::wstring change_encoding<wchar_t, char32_t>(const std::u32string_view&);
#endif  // MH_HAS_CUCHAR

	extern template MH_STUFF_API std::u16string change_encoding<char16_t, char16_t>(const std::u16string_view&);
	extern template MH_STUFF_API std::u16string change_encoding<char16_t, char32_t>(const std::u32string_view&);
	extern template MH_STUFF_API std::u32string change_encoding<char32_t, char16_t>(const std::u16string_view&);
	extern template MH_STUFF_API std::u32string change_encoding<char32_t, char32_t>(const std::u32string_view&);

#if MH_HAS_CHAR8
#if MH_HAS_CUCHAR
	extern template MH_STUFF_API std::u8string change_encoding<char8_t, char>(const std::string_view&);
	extern template MH_STUFF_API std::u8string change_encoding<char8_t, wchar_t>(const std::wstring_view&);

	extern template MH_STUFF_API std::string change_encoding<char, char8_t>(const std::u8string_view&);
	extern template MH_STUFF_API std::wstring change_encoding<wchar_t, char8_t>(const std::u8string_view&);
#endif  // MH_HAS_CUCHAR

	extern template MH_STUFF_API std::u8string change_encoding<char8_t, char8_t>(const std::u8string_view&);
	extern template MH_STUFF_API std::u8string change_encoding<char8_t, char16_t>(const std::u16string_view&);
	extern template MH_STUFF_API std::u8string change_encoding<char8_t, char32_t>(const std::u32string_view&);

	extern template MH_STUFF_API std::u16string change_encoding<char16_t, char8_t>(const std::u8string_view&);

	extern template MH_STUFF_API std::u32string change_encoding<char32_t, char8_t>(const std::u8string_view&);
#endif  // MH_HAS_CHAR8
#endif  // MH_HAS_UNICODE
#endif  // MH_COMPILE_LIBRARY

	template<typename To, typename From>
	inline auto change_encoding(const From* input)
	{
		return change_encoding<To>(std::basic_string_view<From>(input));
	}
	template<typename To, typename From,
		typename FromTraits = std::char_traits<From>,
		typename FromAlloc = std::allocator<From>>
	inline auto change_encoding(const std::basic_string<From, FromTraits, FromAlloc>& input)
	{
		return change_encoding<To>(std::basic_string_view<From, FromTraits>(input));
	}
}

#ifndef MH_COMPILE_LIBRARY
#include "codecvt.inl"
#endif
