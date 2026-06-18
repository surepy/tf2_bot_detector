#pragma once

#include <fmt/format.h>

#include <system_error>

template<typename T, typename CharT>
struct fmt::formatter<T, CharT, std::enable_if_t<std::is_same_v<T, std::error_code> || std::is_same_v<T, std::error_condition>>>
{
	static constexpr CharT PRES_MSG = 'm';
	static constexpr CharT PRES_CATEGORY_NAME = 'c';
	static constexpr CharT PRES_VALUE = 'v';

	bool m_CategoryName = true;
	bool m_Message = true;
	bool m_Value = true;

	constexpr auto parse(fmt::basic_format_parse_context<CharT>& ctx)
	{
		auto it = ctx.begin();
		const auto end = ctx.end();
		if (it != end)
		{
			if (*it != '}')
			{
				m_CategoryName = false;
				m_Message = false;
				m_Value = false;
			}

			for (; it != end; ++it)
			{
				if (*it == PRES_MSG)
					m_Message = true;
				else if (*it == PRES_CATEGORY_NAME)
					m_CategoryName = true;
				else if (*it == PRES_VALUE)
					m_Value = true;
				else if (*it == '}')
					return it;
				else
					throw fmt::format_error(fmt::format("Unexpected character '{}' in formatting string", *it));
			}

			throw fmt::format_error("Unexpected end of format string when looking for '}'");
		}

		return it;
	}

	template<typename FormatContext>
	auto format(const T& ec, FormatContext& ctx) const
	{
		auto it = ctx.out();

		if (m_CategoryName)
			it = fmt::format_to(it, "{}", ec.category().name());
		if (m_Value)
			it = fmt::format_to(it, "({})", ec.value());
		if (m_Message)
		{
			if (m_Value)
				it = fmt::format_to(it, ": ");

			it = fmt::format_to(it, "{}", ec.message());
		}

		return it;
	}
};
