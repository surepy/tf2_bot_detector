#ifdef MH_COMPILE_LIBRARY
#include "codecvt.hpp"
#endif

#include <cassert>
#include <cwchar>
#include <stdexcept>
#include <type_traits>

#ifndef MH_HAS_CUCHAR
#define MH_HAS_CUCHAR (__has_include(<cuchar>))
#endif

#if MH_HAS_CUCHAR
#include <cuchar>
#endif

#ifndef MH_COMPILE_LIBRARY_INLINE
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

namespace mh
{
	namespace detail::codecvt_hpp
	{
		template<typename From, typename To, typename TEnable = void> struct change_encoding_impl;

		template<typename T> constexpr bool is_utf_v =
#if MH_HAS_CHAR8
			std::is_same_v<T, char8_t> ||
#endif
#if MH_HAS_UNICODE
			std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t> ||
#endif
			false;

#if MH_HAS_CHAR8
		[[nodiscard]] MH_COMPILE_LIBRARY_INLINE char32_t convert_to_u32(
			const char8_t*& it, const char8_t* end)
		{
			unsigned continuationBytes = 0;

			uint32_t retVal{};

			// https://en.wikipedia.org/wiki/UTF-8#Examples
			const uint8_t firstByte = uint8_t(*it++);
			if ((firstByte & 0b1111'0000) == 0b1111'0000)
			{
				retVal = uint32_t(0b0000'0111 & firstByte) << 18;
				continuationBytes = 3;
			}
			else if ((firstByte & 0b1110'0000) == 0b1110'0000)
			{
				retVal = uint32_t(0b0000'1111 & firstByte) << 12;
				continuationBytes = 2;
			}
			else if ((firstByte & 0b1100'0000) == 0b1100'0000)
			{
				retVal = uint32_t(0b0001'1111 & firstByte) << 6;
				continuationBytes = 1;
			}
			else //if ((firstByte & 0b1000'0000) == 0b0000'0000)
			{
				return char32_t(0b0111'1111 & firstByte);
				//retVal = 0b0111'1111 & firstByte;
				//continuationBytes = 0;
			}

			// Stop at continuationBytes == 0 or reaching the end iterator, whichever is sooner
			for (; it != end && continuationBytes--; ++it)
			{
				retVal |= uint32_t((*it) & 0b0011'1111) << (6 * continuationBytes);
			}

			return char32_t(retVal);
		}
#endif

#if MH_HAS_UNICODE
		MH_COMPILE_LIBRARY_INLINE char32_t convert_to_u32(const char16_t*& it, const char16_t* end)
		{
			// https://en.wikipedia.org/wiki/UTF-16#Description
			constexpr uint16_t TOP6_MASK = 0xFC00;

			const uint16_t firstByte = uint16_t(*it++);
			if ((firstByte & TOP6_MASK) == 0xD800)
			{
				uint32_t retVal = uint32_t(firstByte & (~TOP6_MASK)) << 10;

				if (it != end)
					retVal |= uint32_t(uint32_t(*it++) & (~TOP6_MASK));

				return 0x10000 + retVal;
			}

			return firstByte;
		}

		template<typename T>
		[[nodiscard]] MH_COMPILE_LIBRARY_INLINE constexpr size_t convert_to_u8(char32_t in, T out[4])
		{
			const uint32_t in_raw = in;

			if (in_raw <= 0x7F)
			{
				out[0] = in_raw & 0b0111'1111;
				return 1;
			}
			else if (in_raw <= 0x7FF)
			{
				out[0] = 0b1100'0000 | ((in_raw >> 6) & 0b0001'1111);
				out[1] = 0b1000'0000 | (in_raw & 0b0011'1111);
				return 2;
			}
			else if (in_raw <= 0xFFFF)
			{
				out[0] = 0b1110'0000 | ((in_raw >> 12) & 0b0000'1111);
				out[1] = 0b1000'0000 | ((in_raw >> 6) & 0b0011'1111);
				out[2] = 0b1000'0000 | ((in_raw >> 0) & 0b0011'1111);
				return 3;
			}
			else if (in_raw <= 0x10FFFF)
			{
				out[0] = 0b1111'0000 | ((in_raw >> 18) & 0b0000'0111);
				out[1] = 0b1000'0000 | ((in_raw >> 12) & 0b0011'1111);
				out[2] = 0b1000'0000 | ((in_raw >> 6) & 0b0011'1111);
				out[3] = 0b1000'0000 | ((in_raw >> 0) & 0b0011'1111);
				return 4;
			}

			return -1;
		}
		MH_COMPILE_LIBRARY_INLINE size_t convert_to_u16(char32_t in, char16_t out[2])
		{
			constexpr uint16_t TOP6_MASK = 0xFC00;
			constexpr uint16_t BYTE0_MARKER = 0xD800;
			constexpr uint16_t BYTE1_MARKER = 0xDC00;

			uint32_t in_raw = in;
			if (in_raw > 0xFFFF || ((in_raw & TOP6_MASK) == BYTE0_MARKER))
			{
				in_raw -= 0x10000;

				// Two bytes
				out[0] = BYTE0_MARKER | ((in_raw >> 10) & (~TOP6_MASK));
				out[1] = BYTE1_MARKER | (in_raw & (~TOP6_MASK));
				return 2;
			}
			else
			{
				// One byte
				out[0] = in_raw & 0xFFFF;
				return 1;
			}
		}

#if MH_HAS_CHAR8
		MH_COMPILE_LIBRARY_INLINE size_t convert_to_uc(char32_t in, std::basic_string<char8_t>& out)
		{
			char8_t buf[4];
			const size_t chars = convert_to_u8(in, buf);
			out.append(buf, chars);
			return chars;
		}
#endif
		MH_COMPILE_LIBRARY_INLINE size_t convert_to_uc(char32_t in, std::basic_string<char16_t>& out)
		{
			char16_t buf[2];
			const size_t chars = convert_to_u16(in, buf);
			out.append(buf, chars);
			return chars;
		}
		MH_COMPILE_LIBRARY_INLINE size_t convert_to_uc(char32_t in, std::basic_string<char32_t>& out)
		{
			out += in;
			return 1;
		}

#if MH_HAS_CUCHAR
		MH_COMPILE_LIBRARY_INLINE std::size_t convert_to_mb(char* buf, char16_t from,
			std::mbstate_t& state)
		{
			return std::c16rtomb(buf, from, &state);
		}
		MH_COMPILE_LIBRARY_INLINE std::size_t convert_to_mb(char* buf, char32_t from,
			std::mbstate_t& state)
		{
			return std::c32rtomb(buf, from, &state);
		}

		MH_COMPILE_LIBRARY_INLINE std::size_t convert_to_utf(char16_t* buf, const char* mb,
			std::size_t mbmax, std::mbstate_t& state)
		{
			return std::mbrtoc16(buf, mb, mbmax, &state);
		}
		MH_COMPILE_LIBRARY_INLINE std::size_t convert_to_utf(char32_t* buf, const char* mb,
			std::size_t mbmax, std::mbstate_t& state)
		{
			return std::mbrtoc32(buf, mb, mbmax, &state);
		}

		template<typename T>
		inline void utf_to_mb(std::string& out, T u32, std::mbstate_t& state)
		{
			static_assert(is_utf_v<T>);
#ifndef MB_LEN_MAX
			constexpr int MB_LEN_MAX = 64;
#endif
			char tempBuf[MB_LEN_MAX];

			const auto bytesWritten = convert_to_mb(tempBuf, u32, state);
			if (bytesWritten == size_t(-1))
				throw std::invalid_argument("Failed to convert all characters");
			else if (bytesWritten > sizeof(tempBuf))
				throw std::runtime_error("Stack corruption");

			out.append(tempBuf, bytesWritten);
		}

#if MH_HAS_CHAR8
		template<>
		struct change_encoding_impl<char8_t, char>
		{
			std::basic_string<char> operator()(const char8_t* begin, const char8_t* end) const
			{
				std::basic_string<char> retVal;

				std::mbstate_t state{};
				for (auto it = begin; it != end; )
				{
					const char32_t u32 = convert_to_u32(it, end);
					utf_to_mb(retVal, u32, state);
				}

				return retVal;
			}
		};
#endif

		template<typename From>
		struct change_encoding_impl<From, char, std::enable_if_t<is_utf_v<From>>>
		{
			std::string operator()(const From* begin, const From* end) const
			{
				std::string retVal;
				std::mbstate_t state{};

				for (auto it = begin; it != end; ++it)
					utf_to_mb(retVal, *it, state);

				return retVal;
			}
		};

		template<typename To>
		struct change_encoding_impl<char, To, std::enable_if_t<!std::is_same_v<char, To>>>
		{
			std::basic_string<To> operator()(const char* begin, const char* end) const
			{
				static_assert(is_utf_v<To>);
				std::basic_string<To> retVal;

				std::mbstate_t state{};
				for (auto it = begin; it != end; )
				{
					char32_t u32;
					const auto result = convert_to_utf(&u32, it, end - it, state);
					if (result == 0)
					{
						// Stored the null character, we're an std::string so we can continue
						retVal += To(0);
						state = {};
						++it;
					}
					else if (result == static_cast<std::size_t>(-3))
					{
						// Another charN_t needs to be written to the output stream
						convert_to_uc(u32, retVal);
					}
					else if (result == static_cast<std::size_t>(-2))
					{
						// FIXME is this correct? "...forms an incomplete, but so far valid, multibyte character"
						// https://en.cppreference.com/w/cpp/string/multibyte/mbrtoc32
						throw std::invalid_argument("Segment forms invalid UTF character sequence");
					}
					else if (result == static_cast<std::size_t>(-1))
					{
						throw std::runtime_error("Encoding error");
					}
					else
					{
						it += result;
						convert_to_uc(u32, retVal);
					}
				}

				return retVal;
			}
		};
#endif  // __has_include(<cuchar>)

		template<typename From, typename To>
		struct change_encoding_impl<From, To, std::enable_if_t<!std::is_same_v<From, To>&& is_utf_v<From>&& is_utf_v<To>>>
		{
			std::basic_string<To> operator()(const From* begin, const From* end) const
			{
				std::basic_string<To> retVal;

				for (auto it = begin; it != end; )
				{
					char32_t u32;

					if constexpr (std::is_same_v<From, char32_t>)
						u32 = *it++;
					else
						u32 = convert_to_u32(it, end);

#if MH_HAS_CHAR8
					if constexpr (std::is_same_v<To, char8_t>)
					{
						char8_t buf[4];
						const size_t chars = convert_to_u8(u32, buf);
						retVal.append(buf, chars);
					}
					else
#endif
						if constexpr (std::is_same_v<To, char16_t>)
					{
						char16_t buf[2];
						const auto chars = convert_to_u16(u32, buf);
						retVal.append(buf, chars);
					}
					else //if constexpr (std::is_same_v<To, char32_t>)
					{
						retVal += u32;
					}
				}

				return retVal;
			}
		};
#endif // MH_HAS_UNICODE

		template<> struct change_encoding_impl<char, wchar_t>
		{
			std::basic_string<wchar_t> operator()(const char* begin, const char* end) const
			{
				std::basic_string<wchar_t> retVal;

				std::mbstate_t state{};
				for (auto it = begin; it != end; )
				{
					wchar_t wc;
					const auto result = std::mbrtowc(&wc, it, end - it, &state);

					if (result == 0)
					{
						// Stored the null character, we're an std::string so we can continue
						retVal += wc;
						state = {};
						++it;
					}
					else if (result == static_cast<std::size_t>(-2))
					{
						throw std::invalid_argument("Segment forms invalid multibyte character sequence");
					}
					else if (result == static_cast<std::size_t>(-1))
					{
						throw std::runtime_error("Encoding error");
					}
					else
					{
						it += result;
						retVal += wc;
					}
				}

				return retVal;
			}
		};

		template<> struct change_encoding_impl<wchar_t, char>
		{
			std::basic_string<char> operator()(const wchar_t* begin, const wchar_t* end) const
			{
				std::basic_string<char> retVal;

				const auto MB_CUR_MAX_VAL = MB_CUR_MAX;
				char* buf = reinterpret_cast<char*>(alloca(MB_CUR_MAX_VAL));
				std::mbstate_t state{};
				for (auto it = begin; it != end; )
				{
					size_t result;
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
					wcrtomb_s(&result, buf, MB_CUR_MAX_VAL, *it, &state);
#else
					result = std::wcrtomb(buf, *it, &state);
#endif

					if (result == static_cast<std::size_t>(-1))
					{
						throw std::invalid_argument("Invalid wide character");
					}
					else
					{
						assert(result != 0);
						it += result;
						retVal.append(buf, result);
					}
				}

				return retVal;
			}
		};

		template<typename From>
		struct change_encoding_impl<From, wchar_t, std::enable_if_t<!std::is_same_v<wchar_t, From>>>
		{
			std::basic_string<wchar_t> operator()(const From* begin, const From* end) const
			{
				auto converted = change_encoding_impl<From, char>{}(begin, end);
				return change_encoding_impl<char, wchar_t>{}(converted.data(), converted.data() + converted.size());
			}
		};

		template<typename To>
		struct change_encoding_impl<wchar_t, To, std::enable_if_t<!std::is_same_v<wchar_t, To>>>
		{
			std::basic_string<To> operator()(const wchar_t* begin, const wchar_t* end) const
			{
				auto converted = change_encoding_impl<wchar_t, char>{}(begin, end);
				return change_encoding_impl<char, To>{}(converted.data(), converted.data() + converted.size());
			}
		};
		template<typename T>
		struct change_encoding_impl<T, T, std::enable_if_t<std::is_same_v<T, T>>>
		{
			std::basic_string<T> operator()(const T* begin, const T* end) const
			{
				return std::basic_string<T>(begin, end - begin);
			};
		};
	}

	template<typename To, typename From, typename FromTraits>
	MH_COMPILE_LIBRARY_INLINE std::basic_string<To> change_encoding(const std::basic_string_view<From, FromTraits>& input)
	{
		const detail::codecvt_hpp::template change_encoding_impl<From, To> impl;
		return impl(input.data(), input.data() + input.size());
	}

#ifdef MH_COMPILE_LIBRARY
	template std::string change_encoding<char, char>(const std::string_view&);
	template std::string change_encoding<char, wchar_t>(const std::wstring_view&);

	template std::wstring change_encoding<wchar_t, char>(const std::string_view&);
	template std::wstring change_encoding<wchar_t, wchar_t>(const std::wstring_view&);

#if MH_HAS_UNICODE
#if MH_HAS_CUCHAR
	template std::u16string change_encoding<char16_t, char>(const std::string_view&);
	template std::u16string change_encoding<char16_t, wchar_t>(const std::wstring_view&);
	template std::u32string change_encoding<char32_t, char>(const std::string_view&);
	template std::u32string change_encoding<char32_t, wchar_t>(const std::wstring_view&);

	template std::string change_encoding<char, char16_t>(const std::u16string_view&);
	template std::string change_encoding<char, char32_t>(const std::u32string_view&);
	template std::wstring change_encoding<wchar_t, char16_t>(const std::u16string_view&);
	template std::wstring change_encoding<wchar_t, char32_t>(const std::u32string_view&);
#endif  // MH_HAS_CUCHAR

	template std::u16string change_encoding<char16_t, char16_t>(const std::u16string_view&);
	template std::u16string change_encoding<char16_t, char32_t>(const std::u32string_view&);
	template std::u32string change_encoding<char32_t, char16_t>(const std::u16string_view&);
	template std::u32string change_encoding<char32_t, char32_t>(const std::u32string_view&);

#if MH_HAS_CHAR8
#if MH_HAS_CUCHAR
	template std::u8string change_encoding<char8_t, char>(const std::string_view&);
	template std::u8string change_encoding<char8_t, wchar_t>(const std::wstring_view&);

	template std::string change_encoding<char, char8_t>(const std::u8string_view&);
	template std::wstring change_encoding<wchar_t, char8_t>(const std::u8string_view&);
#endif  // MH_HAS_CUCHAR

	template std::u8string change_encoding<char8_t, char8_t>(const std::u8string_view&);
	template std::u8string change_encoding<char8_t, char16_t>(const std::u16string_view&);
	template std::u8string change_encoding<char8_t, char32_t>(const std::u32string_view&);

	template std::u16string change_encoding<char16_t, char8_t>(const std::u8string_view&);

	template std::u32string change_encoding<char32_t, char8_t>(const std::u8string_view&);
#endif  // MH_HAS_CHAR8
#endif  // MH_HAS_UNICODE
#endif  // MH_COMPILE_LIBRARY
}
