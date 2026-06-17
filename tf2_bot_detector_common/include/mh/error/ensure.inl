#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(_MSC_VER) && defined(_DEBUG) // _CrtDbgReport
#include <crtdbg.h>
#endif

#ifdef MH_COMPILE_LIBRARY
#include "mh/error/ensure.hpp"
#else
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

namespace mh::detail::ensure_hpp
{
	MH_COMPILE_LIBRARY_INLINE void ensure_traits_default_base::print_details_label(std::ostream& os, const char* label) const
	{
		os << "\n\t" << std::setw(get_max_details_width()) << std::right << label << " :  ";
	}

	MH_COMPILE_LIBRARY_INLINE ensure_trigger_result ensure_traits_default_base::trigger_generic(const ensure_info_base& info) const
	{
		std::ostringstream ss;
		ss << "mh_ensure failed: " << info.m_ExpressionText;

		print_details_label(ss, "in");
		ss << info.m_Location.function_name();

		print_details_label(ss, "at");
		ss << info.m_Location.file_name() << ':' << info.m_Location.line();

		if (info.m_Message && info.m_Message[0])
		{
			print_details_label(ss, "message");
			ss << info.m_Message;
		}

		if (can_print_value())
		{
			print_details_label(ss, "value");
			print_value_generic(ss, info);
		}

		std::cerr << ss.str() << std::endl;

#if defined(_MSC_VER) && defined(_DEBUG)
		const auto userResult = _CrtDbgReport(_CRT_ASSERT, info.m_Location.file_name(), info.m_Location.line(), nullptr, "%s", ss.str().c_str());
		if (userResult == 1 || userResult == -1) // -1 = error, 1 = user clicked "break" (retry)
			return ensure_trigger_result::debugger_break;
#endif

		return ensure_trigger_result::ignore;
	}
}
