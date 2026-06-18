#pragma once

#include <string>
#include <string_view>
#include <ostream>

namespace mh
{
	namespace detail::insertion_conversion_hpp
	{
		template<typename T> static constexpr bool is_char_v =
			std::is_same_v<T, char> || std::is_same_v<T, wchar_t> || std::is_same_v<T, signed char> || std::is_same_v<T, unsigned char>
#if __cpp_char8_t >= 201811
			|| std::is_same_v<T, char8_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>
#endif
			;
	}
}

template<typename CharT, typename Traits, typename CharT2, typename Traits2, typename Alloc,
	typename = std::enable_if_t<!std::is_same_v<CharT, CharT2> && mh::detail::insertion_conversion_hpp::is_char_v<CharT2>>>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const std::basic_string<CharT2, Traits2, Alloc>& str)
{
	return os << str.c_str();
}

template<typename CharT, typename Traits, typename CharT2, typename Traits2,
	typename = std::enable_if_t<!std::is_same_v<CharT, CharT2> && mh::detail::insertion_conversion_hpp::is_char_v<CharT2>>>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const std::basic_string_view<CharT2, Traits2>& str)
{
	for (auto ch : str)
		os.put(ch);

	return os;
}
