#pragma once

#include <cctype>
#include <ostream>
#include <string>
#include <string_view>

namespace mh
{
	template<typename BaseTraits = std::char_traits<char>>
	class case_insensitive_char_traits : public BaseTraits
	{
	public:
		using char_type = typename BaseTraits::char_type;
		using int_type = typename BaseTraits::int_type;
		using off_type = typename BaseTraits::off_type;
		using pos_type = typename BaseTraits::pos_type;
		using state_type = typename BaseTraits::state_type;

		case_insensitive_char_traits() = default;
		case_insensitive_char_traits(const BaseTraits& base) : BaseTraits(base) {}
		case_insensitive_char_traits(BaseTraits&& base) : BaseTraits(base) {}

		static int compare(const char_type* s1, const char_type* s2, size_t count)
		{
			while (count--)
			{
				const auto c1 = std::toupper(*s1);
				const auto c2 = std::toupper(*s2);

				s1++;
				s2++;

				if (c1 == c2)
					continue;

				if (c1 < c2)
					return -1;
				if (c1 > c2)
					return 1;
			}

			return 0;
		}

		template<typename Traits1, typename Traits2>
		static int compare(const std::basic_string_view<char_type, Traits1>& s1, const std::basic_string_view<char_type, Traits2>& s2)
		{
			if (auto result = compare(s1.data(), s2.data(), s1.size() < s2.size() ? s1.size() : s2.size()); result != 0)
				return result;

			if (s1.size() < s2.size())
				return -1;
			if (s1.size() > s2.size())
				return 1;

			return 0;
		}

		static bool eq(char_type c1, char_type c2) { return std::toupper(c1) == std::toupper(c2); }
		static bool lt(char_type c1, char_type c2) { return std::toupper(c1) < std::toupper(c2); }

		static const char_type* find(const char_type* p, size_t count, const char_type& ch)
		{
			while (count-- && *p)
			{
				if (*p == ch)
					return p;

				p++;
			}

			return nullptr;
		}
	};

#ifdef MH_COMPILE_LIBRARY
	extern template class case_insensitive_char_traits<std::char_traits<char>>;
	extern template class case_insensitive_char_traits<std::char_traits<wchar_t>>;
#endif

	template<typename CharT, typename Traits>
	auto case_insensitive_view(const std::basic_string_view<CharT, Traits>& sv)
	{
		return std::basic_string_view<CharT, case_insensitive_char_traits<Traits>>(sv.data(), sv.size());
	}
	template<typename CharT, typename BaseTraits = std::char_traits<CharT>>
	auto case_insensitive_view(const CharT* str)
	{
		return std::basic_string_view<CharT, case_insensitive_char_traits<BaseTraits>>(str);
	}
	template<typename CharT, typename BaseTraits = std::char_traits<CharT>>
	auto case_insensitive_view(const CharT* str, size_t length)
	{
		return std::basic_string_view<CharT, case_insensitive_char_traits<BaseTraits>>(str, length);
	}
	template<typename CharT, typename Traits, typename Alloc>
	auto case_insensitive_view(const std::basic_string<CharT, Traits, Alloc>& str)
	{
		return case_insensitive_view(std::basic_string_view<CharT, Traits>(str));
	}

	template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	auto case_insensitive_string(const std::basic_string<CharT, Traits, Alloc>& str)
	{
		return std::basic_string<CharT, case_insensitive_char_traits<Traits>, Alloc>(str.data(), str.size(), str.get_allocator());
	}
	template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	auto case_insensitive_string(const std::basic_string_view<CharT, Traits>& str)
	{
		return std::basic_string<CharT, case_insensitive_char_traits<Traits>, Alloc>(str.data(), str.size());
	}
	template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	auto case_insensitive_string(const CharT* str)
	{
		return case_insensitive_string<CharT, Traits, Alloc>(std::basic_string_view<CharT, Traits>(str));
	}

	template<typename CharT = char, typename TraitsLHS = std::char_traits<CharT>, typename TraitsRHS = std::char_traits<CharT>>
	bool case_insensitive_compare(
		const std::basic_string_view<CharT, TraitsLHS>& lhs,
		const std::basic_string_view<CharT, TraitsRHS>& rhs)
	{
		return case_insensitive_view(lhs) == case_insensitive_view(rhs);
	}
	template<typename T0, typename T1,
		typename CharT0 = typename T0::value_type, typename TraitsT0 = typename T0::traits_type,
		typename CharT1 = typename T1::value_type, typename TraitsT1 = typename T1::traits_type>
		bool case_insensitive_compare(const T0& lhs, const T1& rhs)
	{
		return case_insensitive_compare(std::basic_string_view<CharT0, TraitsT0>(lhs), std::basic_string_view<CharT1, TraitsT1>(rhs));
	}
}

#if 0
inline namespace test
{
	template<typename CharT, typename Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
		const std::basic_string_view<CharT, mh::case_insensitive_char_traits<std::char_traits<CharT>>>& str)
	{
		return os.write(str.data(), str.size());
	}
	template<typename CharT, typename Traits>
	std::basic_ostream<CharT, Traits>&& operator<<(std::basic_ostream<CharT, Traits>&& os,
		const std::basic_string_view<CharT, mh::case_insensitive_char_traits<std::char_traits<CharT>>>& str)
	{
		os.write(str.data(), str.size());
		return std::move(os);
	}

	template<typename CharT, typename Traits, typename Alloc>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
		const std::basic_string<CharT, mh::case_insensitive_char_traits<std::char_traits<CharT>>, Alloc>& str)
	{
		return os.write(str.data(), str.size());
	}
	template<typename CharT, typename Traits, typename Alloc>
	std::basic_ostream<CharT, Traits>&& operator<<(std::basic_ostream<CharT, Traits>&& os,
		const std::basic_string<CharT, mh::case_insensitive_char_traits<std::char_traits<CharT>>, Alloc>& str)
	{
		os.write(str.data(), str.size());
		return std::move(os);
	}
}
#endif
