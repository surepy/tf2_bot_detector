#pragma once

#if __has_include(<getopt.h>) || __has_include(<uinstd.h>)
#if __has_include(<getopt.h>)
#include <getopt.h>
#elif __has_include(<unistd.h>)
#include <unistd.h>
#endif

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>


#if __has_include(<mh/data/variable_pusher.hpp>)
#include <mh/data/variable_pusher.hpp>
namespace mh::detail::getopt_hpp
{
	template<typename T> using variable_pusher = mh::variable_pusher<T>;
}
#else
namespace mh::detail::getopt_hpp
{
	template<typename T>
	class variable_pusher
	{
	public:
		constexpr variable_pusher(T& variable, const T& new_value) :
			m_Variable(variable), m_OldValue(std::move(variable))
		{
			m_Variable = new_value;
		}
		~variable_pusher()
		{
			m_Variable = std::move(m_OldValue);
		}
	private:
		T& m_Variable;
		T m_OldValue;
	}
}
#endif

namespace mh
{
	struct parsed_option
	{
	private:
		static constexpr option DEFAULTED_OPTION{};
	public:
		static constexpr int UNKNOWN_OPT_RESULT = '?';

		constexpr parsed_option() = default;
		constexpr parsed_option(int getopt_result_, const std::string_view& arg_value_ = {}) :
			getopt_result(getopt_result_),
			arg_value(arg_value_)
		{
		}
		constexpr parsed_option(int getopt_result_, const std::string_view& arg_value_, bool is_non_option_,
			const option* longopts_, int longopt_index_) :
			getopt_result(getopt_result_), arg_value(arg_value_),
			is_non_option(is_non_option_),
			longopts(longopts_), longopt_index(longopt_index_)
		{
		}

		std::string get_arg_name() const
		{
			const auto an_long = get_arg_name_long();
			if (!an_long.empty())
				return std::string(an_long);
			else
				return std::string(1, get_arg_name_short());
		}
		constexpr std::string_view get_arg_name_long() const
		{
			auto longopt = get_longopt();
			return longopt ? std::string_view(longopt->name) : std::string_view();
		}
		constexpr char get_arg_name_short() const
		{
			if (getopt_result == '?')
				return optopt;
			else
				return getopt_result;
		}

		constexpr const option* get_longopt(const option* defaultVal = nullptr) const
		{
			if (longopts && longopt_index >= 0)
				return &longopts[longopt_index];

			return defaultVal;
		}
		constexpr const option& get_longopt_safe(const option& defaultVal = DEFAULTED_OPTION) const
		{
			return *get_longopt(&defaultVal);
		}

		int getopt_result = 0;
		std::string_view arg_value{};
		bool is_non_option = false;

		const option* longopts = nullptr;
		int longopt_index = -1;
	};

	template<typename TFunc>
	[[nodiscard]] bool parse_args(int argc, char* const argv[], const char* shortopt,
		const option* longopt_begin, const option* longopt_end,
		const TFunc& func)
	{
		const detail::getopt_hpp::variable_pusher<int> optind_pusher(optind, 1);

		const bool using_longopts = longopt_begin || longopt_end;
		if (using_longopts)
		{
			if ((longopt_end - longopt_begin) < 1)
				throw std::logic_error("longopt array must have at least 1 element in order to null-terminate it");
			if (auto end = (longopt_end - 1); end->flag != 0 || end->name != 0 || end->has_arg != 0 || end->val != 0)
				throw std::logic_error("longopt array must be null-terminated");
		}

		while (true)
		{
			parsed_option parsed{};
			parsed.longopts = longopt_begin;

			if (using_longopts)
				parsed.getopt_result = getopt_long(argc, argv, shortopt, longopt_begin, &parsed.longopt_index);
			else
				parsed.getopt_result = getopt(argc, argv, shortopt);

			if (parsed.getopt_result == -1)
				break;

			if (optarg)
				parsed.arg_value = optarg;

			static_assert(std::is_same_v<decltype(func(parsed)), bool>);
			if (!func(parsed))
			{
				return false;
				//std::cerr << "Unknown argument " << std::quoted()
			}
		}

		while (optind < argc)
		{
			parsed_option parsed{};
			parsed.arg_value = argv[optind];
			parsed.is_non_option = true;
			func(parsed);
			optind++;
		}

		return true;
	}

	template<typename TFunc, size_t longopt_count>
	[[nodiscard]] bool parse_args(int argc, char* const* argv, const char* shortopt,
		const option (&longopts)[longopt_count], const TFunc& func)
	{
		return parse_args<TFunc>(argc, argv, shortopt,
			longopts, longopts + longopt_count,
			func);
	}

	template<typename TFunc>
	[[nodiscard]] bool parse_args(int argc, char* const* argv, const char* shortopt, const TFunc& func)
	{
		return parse_args<TFunc>(argc, argv, shortopt, nullptr, nullptr, func);
	}
}

#ifndef MH_GETOPT_DISABLE_OPTION_OSTREAM
template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const option& opt)
{
	os << "{ ";

	if (opt.name)
		os << '"' << opt.name << '"';
	else
		os << "nullptr";

	os << ", ";

	switch (opt.has_arg)
	{
		case no_argument: os << "no_argument"; break;
		case required_argument: os << "required_argument"; break;
		case optional_argument: os << "optional_argument"; break;
		default:
			os << +opt.has_arg;
			break;
	}

	os << ", ";

	if (opt.flag)
		os << '&' << *opt.flag;
	else
		os << "nullptr";

	os << ", " << opt.val;

	return os << " }";
}
#endif

#if !defined(MH_GETOPT_DISABLE_OPTION_COMPARISON)
#if (__cpp_impl_three_way_comparison >= 201907) && (__cpp_lib_three_way_comparison >= 201907)
inline constexpr std::strong_ordering operator<=>(const option& lhs, const option& rhs)
{
	if (lhs.name && rhs.name)
	{
		if (auto result = std::string_view(lhs.name) <=> std::string_view(rhs.name); std::is_neq(result))
			return result;
	}
	else if (auto result = lhs.name <=> rhs.name; std::is_neq(result))
		return result;

	if (auto result = lhs.has_arg <=> rhs.has_arg; std::is_neq(result))
		return result;
	if (auto result = lhs.flag <=> rhs.flag; std::is_neq(result))
		return result;
	if (auto result = lhs.val <=> rhs.val; std::is_neq(result))
		return result;

	return std::strong_ordering::equal;
}
#else
inline constexpr bool operator==(const option& lhs, const option& rhs)
{
	if (lhs.name && rhs.name)
	{
		if (std::string_view(lhs.name) != std::string_view(rhs.name))
			return false;
	}
	else if (auto result = (lhs.name == rhs.name); !result)
		return false;

	if (lhs.has_arg != rhs.has_arg)
		return false;
	if (lhs.flag != rhs.flag)
		return false;
	if (lhs.val != rhs.val)
		return false;

	return true;
}
inline constexpr bool operator!=(const option& lhs, const option& rhs) { return !operator==(lhs, rhs); }
#endif
#endif

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const mh::parsed_option& opt)
{
	os << "{\n\tgetopt_result = " << opt.getopt_result;

	if (isprint(opt.getopt_result))
		os << " ('" << char(opt.getopt_result) << "')";

	os << "\n\tget_arg_name() = " << std::quoted(opt.get_arg_name());
	os << "\n\tget_arg_name_long() = " << std::quoted(opt.get_arg_name_long());

	os << "\n\tget_arg_name_short() = '" << opt.get_arg_name_short()
		<< "'\n\targ_value = " << std::quoted(opt.arg_value);

	os << "\n\tlongopt_index = " << opt.longopt_index;

#ifndef MH_GETOPT_DISABLE_OPTION_OSTREAM
	os << "\n\tlongopt = " << opt.get_longopt_safe();
#else
	os << "\n\tlongopt = " << (void*)opt.get_longopt();
#endif

	os << "\n\tis_non_option = " << std::boolalpha << opt.is_non_option;

	return os << "\n}";
}

#endif
