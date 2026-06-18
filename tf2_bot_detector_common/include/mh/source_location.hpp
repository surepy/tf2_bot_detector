#pragma once

#include <cstdint>
#include <ostream>

#if __has_include(<source_location>)
#include <source_location>
#endif

namespace mh
{
#if __cpp_lib_source_location >= 201907

	using source_location = std::source_location;
#define MH_SOURCE_LOCATION_CURRENT() ::mh::source_location::current()
#define MH_SOURCE_LOCATION_AUTO(varName) const ::mh::source_location& varName = ::mh::source_location::current()

#else

	class source_location final
	{
	public:
		constexpr source_location() noexcept = default;
		explicit constexpr source_location(std::uint_least32_t line, const char* fileName, const char* functionName, std::uint_least32_t column = 0) noexcept :
			m_Line(line), m_Column(column), m_FileName(fileName), m_FunctionName(functionName)
		{
		}

#if _MSC_VER >= 1927
		static constexpr source_location current(std::uint_least32_t line = __builtin_LINE(), const char* fileName = __builtin_FILE(),
			const char* functionName = __builtin_FUNCTION(), std::uint_least32_t column = __builtin_COLUMN()) noexcept
		{
			return source_location(line, fileName, functionName, column);
		}
#endif

		constexpr std::uint_least32_t line() const noexcept { return m_Line; }
		constexpr std::uint_least32_t column() const noexcept { return m_Column; }
		constexpr const char* file_name() const noexcept { return m_FileName; }
		constexpr const char* function_name() const noexcept { return m_FunctionName; }

	private:
		std::uint_least32_t m_Line{};
		std::uint_least32_t m_Column{};
		const char* m_FileName = nullptr;
		const char* m_FunctionName = nullptr;
	};

#if _MSC_VER >= 1927
#define MH_SOURCE_LOCATION_CURRENT() ::mh::source_location::current()
#define MH_SOURCE_LOCATION_AUTO(varName) const ::mh::source_location& varName = ::mh::source_location::current()
#else
#define MH_SOURCE_LOCATION_CURRENT() ::mh::source_location(__LINE__, __FILE__, __func__)
#define MH_SOURCE_LOCATION_AUTO(varName) const ::mh::source_location& varName
#endif

#endif

	template<typename CharT, typename Traits>
	inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const mh::source_location& location)
	{
		return os << location.file_name() << '(' << location.line() << "):" << location.function_name();
	}
}

#include <fmt/format.h>
#include <mh/text/multi_char.hpp>

template<typename CharT>
struct fmt::formatter<mh::source_location, CharT>
{
	static constexpr mh::multi_char PRES_PATH_FULL = mh_make_multi_char(P);
	static constexpr mh::multi_char PRES_PATH_SHORT = mh_make_multi_char(p);
	static constexpr mh::multi_char PRES_LINE = mh_make_multi_char(l);
	static constexpr mh::multi_char PRES_FUNCTION = mh_make_multi_char(f);

	enum class PathPresentation
	{
		None,
		Short,
		Full,

	} m_Path = PathPresentation::Short;

	bool m_Line = true;
	bool m_Function = true;

	constexpr auto parse(fmt::basic_format_parse_context<CharT>& ctx)
	{
		constexpr mh::multi_char CLOSE_BRACE = mh_make_multi_char(});
		auto it = ctx.begin();
		const auto end = ctx.end();
		if (it != end)
		{
			if (*it != CLOSE_BRACE.get<CharT>())
			{
				m_Path = PathPresentation::None;
				m_Line = false;
				m_Function = false;
			}

			for (; it != end; ++it)
			{
				if (*it == PRES_PATH_FULL || *it == PRES_PATH_SHORT)
				{
					if (m_Path != PathPresentation::None)
					{
						throw fmt::format_error(fmt::format("Path presentation was already set: '{}' and '{}' are mutually exclusive",
							PRES_PATH_SHORT.narrow, PRES_PATH_FULL.narrow));
					}

					if (*it == PRES_PATH_SHORT)
						m_Path = PathPresentation::Short;
					else
						m_Path = PathPresentation::Full;
				}
				else if (*it == PRES_LINE)
					m_Line = true;
				else if (*it == PRES_FUNCTION)
					m_Function = true;
				else if (*it == CLOSE_BRACE.get<CharT>())
					return it;
				else
				{
					// Can't print invalid character because fmt does not handle conversion between char types on its own unfortunately
					throw fmt::format_error(fmt::format("Unexpected character in formatting string"));
				}
			}

			throw fmt::format_error("Unexpected end of format string when looking for '}'");
		}

		return it;
	}

	template<typename FormatContext>
	auto format(const mh::source_location& loc, FormatContext& ctx) const
	{
		const auto path = [&]() -> std::string_view
		{
			if (m_Path == PathPresentation::None)
			{
				return {};
			}
			else if (m_Path == PathPresentation::Short)
			{
				const auto view = std::string_view(loc.file_name());
				const auto found = view.find_last_of("\\/");
				if (found == view.npos)
					return view;

				return view.substr(found + 1);
			}
			else if (m_Path == PathPresentation::Full)
			{
				return loc.file_name();
			}
			else
			{
				throw fmt::format_error(fmt::format("Unexpected PathPresentation value {}",
					+std::underlying_type_t<PathPresentation>(m_Path)));
			}
		}();

		char fmtStrBuf[64];
		fmtStrBuf[0] = '\0';
		size_t fmtStrPos = 0;

		if (!path.empty())
		{
			fmtStrBuf[fmtStrPos++] = '{';
			fmtStrBuf[fmtStrPos++] = '0';
			fmtStrBuf[fmtStrPos++] = '}';
		}

		if (m_Line)
		{
			fmtStrBuf[fmtStrPos++] = '(';
			fmtStrBuf[fmtStrPos++] = '{';
			fmtStrBuf[fmtStrPos++] = '1';
			fmtStrBuf[fmtStrPos++] = '}';
			fmtStrBuf[fmtStrPos++] = ')';
		}

		if (m_Function)
		{
			if (fmtStrPos != 0)
				fmtStrBuf[fmtStrPos++] = ':';

			fmtStrBuf[fmtStrPos++] = '{';
			fmtStrBuf[fmtStrPos++] = '2';
			fmtStrBuf[fmtStrPos++] = '}';
		}

		fmtStrBuf[fmtStrPos] = '\0';

		return fmt::format_to(ctx.out(), fmt::runtime(fmtStrBuf), path, loc.line(), loc.function_name());
	}
};
