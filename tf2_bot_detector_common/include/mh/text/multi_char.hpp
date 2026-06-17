#pragma once

#include <climits>
#include <cstdint>
#include <type_traits>

#if __cpp_consteval >= 201811
#define MH_CONSTEVAL consteval
#else
#define MH_CONSTEVAL constexpr
#endif

namespace mh
{
	// Stores all the possible compile-time representations of a character.
	struct multi_char final
	{
		MH_CONSTEVAL multi_char(
			const char(&narrow_)[2],
			const wchar_t(&wide_)[2],
#if __cpp_char8_t >= 201811
			const char8_t(&u8_)[2],
#endif
			const char16_t(&u16_)[2],
			const char32_t(&u32_)[2]) :
			narrow(narrow_[0]),
			wide(wide_[0]),
#if __cpp_char8_t >= 201811
			u8(u8_[0]),
#endif
			u16(u16_[0]),
			u32(u32_[0])
		{
		}

		char narrow;
		wchar_t wide;
#if __cpp_char8_t >= 201811
		char8_t u8;
#endif
		char16_t u16;
		char32_t u32;

		template<typename T> constexpr auto get() const
		{
			using type = std::decay_t<std::remove_pointer_t<std::decay_t<T>>>;
			if constexpr (std::is_same_v<type, char>)
				return narrow;
			else if constexpr (std::is_same_v<type, wchar_t>)
				return wide;
#if __cpp_char8_t >= 201811
			else if constexpr (std::is_same_v<type, char8_t>)
				return u8;
#endif
			else if constexpr (std::is_same_v<type, char16_t>)
				return u16;
			else if constexpr (std::is_same_v<type, char32_t>)
				return u32;
		}
	};

	inline constexpr auto operator==(const mh::multi_char& lhs, char rhs) { return lhs.narrow == rhs; }
	inline constexpr auto operator==(char lhs, const mh::multi_char& rhs) { return lhs == rhs.narrow; }
	inline constexpr auto operator==(const mh::multi_char& lhs, wchar_t rhs) { return lhs.wide == rhs; }
	inline constexpr auto operator==(wchar_t lhs, const mh::multi_char& rhs) { return lhs == rhs.wide; }
#if __cpp_char8_t >= 201811
	inline constexpr auto operator==(const mh::multi_char& lhs, char8_t rhs) { return lhs.u8 == rhs; }
	inline constexpr auto operator==(char8_t lhs, const mh::multi_char& rhs) { return lhs == rhs.u8; }
#endif
	inline constexpr auto operator==(const mh::multi_char& lhs, char16_t rhs) { return lhs.u16 == rhs; }
	inline constexpr auto operator==(char16_t lhs, const mh::multi_char& rhs) { return lhs == rhs.u16; }
	inline constexpr auto operator==(const mh::multi_char& lhs, char32_t rhs) { return lhs.u32 == rhs; }
	inline constexpr auto operator==(char32_t lhs, const mh::multi_char& rhs) { return lhs == rhs.u32; }

#if __cpp_char8_t >= 201811
#define mh_make_multi_char(c) ::mh::multi_char(#c, L ## #c, u8 ## #c, u ## #c, U ## #c)
#else
#define mh_make_multi_char(c) ::mh::multi_char(#c, L ## #c, u ## #c, U ## #c)
#endif
}

#if __has_include(<compare>)
#include <compare>
namespace mh
{
	inline constexpr auto operator<=>(const mh::multi_char& lhs, char rhs) { return lhs.narrow <=> rhs; }
	inline constexpr auto operator<=>(char lhs, const mh::multi_char& rhs) { return lhs <=> rhs.narrow; }
	inline constexpr auto operator<=>(const mh::multi_char& lhs, wchar_t rhs) { return lhs.wide <=> rhs; }
	inline constexpr auto operator<=>(wchar_t lhs, const mh::multi_char& rhs) { return lhs <=> rhs.wide; }
#if __cpp_char8_t >= 201811
	inline constexpr auto operator<=>(const mh::multi_char& lhs, char8_t rhs) { return lhs.u8 <=> rhs; }
	inline constexpr auto operator<=>(char8_t lhs, const mh::multi_char& rhs) { return lhs <=> rhs.u8; }
#endif
	inline constexpr auto operator<=>(const mh::multi_char& lhs, char16_t rhs) { return lhs.u16 <=> rhs; }
	inline constexpr auto operator<=>(char16_t lhs, const mh::multi_char& rhs) { return lhs <=> rhs.u16; }
	inline constexpr auto operator<=>(const mh::multi_char& lhs, char32_t rhs) { return lhs.u32 <=> rhs; }
	inline constexpr auto operator<=>(char32_t lhs, const mh::multi_char& rhs) { return lhs <=> rhs.u32; }
}
#endif
