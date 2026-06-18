#pragma once

#include <initializer_list>
#include <locale>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mh
{
	namespace detail::stringops_hpp
	{
		template<typename CharT> inline static constexpr std::initializer_list<CharT> WHITESPACE_CHARS =
		{
			CharT(' '),
			CharT('\n'),
			CharT('\r'),
			CharT('\t'),
			CharT('\v'),
			CharT('\f'),
		};

		template<typename TString> constexpr typename TString::value_type value_type_helper(int) {}
		template<typename TString> constexpr std::decay_t<decltype(std::declval<TString>()[0])> value_type_helper(void*) {}
		template<typename TString> using value_type_t = decltype(value_type_helper<TString>(0));

		template<typename TString> constexpr typename TString::traits_type traits_type_helper(int) {}
		template<typename TString> constexpr std::char_traits<value_type_t<TString>> traits_type_helper(void*) {}
		template<typename TString> using traits_type_t = decltype(traits_type_helper<TString>(0));

		template<typename TString> constexpr typename TString::allocator_type allocator_type_helper(int) {}
		template<typename TString> constexpr std::allocator<value_type_t<TString>> allocator_type_helper(void*) {}
		template<typename TString> using allocator_type_t = decltype(allocator_type_helper<TString>(0));

		template<typename TString> using string_type_t = std::basic_string<value_type_t<TString>, traits_type_t<TString>, allocator_type_t<TString>>;
		template<typename TString> using string_view_type_t = std::basic_string_view<value_type_t<TString>, traits_type_t<TString>>;
		//template<typename TString> using allocator_type_t =
		//template<typename TString> using string_type = std::basic_string<value_type<TString>, traits_type<TString>,
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] std::basic_string<CharT, Traits, Alloc> trim_start(
		std::basic_string<CharT, Traits, Alloc>&& str, std::initializer_list<CharT> trimChars)
	{
		size_t charsToTrim = 0;
		for (CharT c : str)
		{
			bool found = false;
			for (CharT trimChar : trimChars)
			{
				if (c == trimChar)
				{
					found = true;
					break;
				}
			}

			if (!found)
				break;

			charsToTrim++;
		}

		str.erase(0, charsToTrim);
		return std::move(str);
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> trim_start(std::basic_string<CharT, Traits, Alloc>&& str)
	{
		return mh::trim_start(std::move(str), mh::detail::stringops_hpp::WHITESPACE_CHARS<CharT>);
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] std::basic_string<CharT, Traits, Alloc> trim_end(
		std::basic_string<CharT, Traits, Alloc>&& str, std::initializer_list<CharT> trimChars)
	{
		size_t charsToTrim = 0;
		for (size_t i = str.size(); i--; )
		{
			CharT c = str[i];

			bool found = false;
			for (CharT trimChar : trimChars)
			{
				if (c == trimChar)
				{
					found = true;
					break;
				}
			}

			if (!found)
				break;

			charsToTrim++;
		}

		str.erase(str.size() - charsToTrim);
		return std::move(str);
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> trim_end(std::basic_string<CharT, Traits, Alloc>&& str)
	{
		return mh::trim_end(std::move(str), mh::detail::stringops_hpp::WHITESPACE_CHARS<CharT>);
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> trim(std::basic_string<CharT, Traits, Alloc>&& str, std::initializer_list<CharT> trimChars)
	{
		return mh::trim_start(mh::trim_end(std::move(str), trimChars), trimChars);
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> trim(std::basic_string<CharT, Traits, Alloc>&& str)
	{
		return mh::trim(std::move(str), mh::detail::stringops_hpp::WHITESPACE_CHARS<CharT>);
	}

	template<typename CharT, typename Traits, typename Alloc>
	[[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> find_and_replace(
		std::basic_string<CharT, Traits, Alloc> str, const std::basic_string_view<CharT, Traits>& find,
		const std::basic_string_view<CharT, Traits>& replace)
	{
		size_t curIndex = 0;
		while (true)
		{
			curIndex = str.find(find, curIndex);
			if (curIndex == str.npos)
				break;

			str.replace(curIndex, find.size(), replace);
			curIndex += replace.size();
		}

		return std::move(str);
	}

	template<typename CharT, typename Traits, typename Alloc = std::allocator<CharT>>
	[[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> find_and_replace(
		const std::basic_string_view<CharT, Traits>& str, const std::basic_string_view<CharT, Traits>& find,
		const std::basic_string_view<CharT, Traits>& replace)
	{
		return find_and_replace<CharT, Traits, Alloc>(std::basic_string<CharT, Traits, Alloc>(str), find, replace);
	}

	template<typename TString, typename TRet = detail::stringops_hpp::string_type_t<TString>>
	[[nodiscard]] inline TRet toupper(TString&& str)
	{
		TRet retVal(std::forward<TString>(str));

		const auto& loc = std::locale::classic();
		for (auto& c : retVal)
			c = std::toupper(c, loc);

		return retVal;
	}

	template<typename TString, typename TRet = detail::stringops_hpp::string_type_t<TString>>
	[[nodiscard]] inline TRet tolower(TString&& str)
	{
		TRet retVal(std::forward<TString>(str));

		const auto& loc = std::locale::classic();
		for (auto& c : retVal)
			c = std::tolower(c, loc);

		return retVal;
	}
}

#if __has_include(<mh/coroutine/generator.hpp>)
#include <mh/coroutine/generator.hpp>
#ifdef MH_COROUTINES_SUPPORTED
namespace mh
{
	template<typename CharT, typename Traits>
	[[nodiscard]] mh::generator<std::basic_string_view<CharT, Traits>> split_string(
		const std::basic_string_view<CharT, Traits>& string, const std::basic_string_view<CharT, Traits>& splitChars)
	{
		size_t lastEnd = 0;
		while (lastEnd != string.npos)
		{
			const size_t found = string.find_first_of(splitChars, lastEnd);
			co_yield string.substr(lastEnd, found - lastEnd);
			lastEnd = found;
		}
	}

	template<typename TStr1, typename TStr2>
	[[nodiscard]] auto split_string(const TStr1& string, const TStr2& splitChars)
	{
		return split_string(std::basic_string_view(string), std::basic_string_view(splitChars));
	}
}
#endif
#endif
