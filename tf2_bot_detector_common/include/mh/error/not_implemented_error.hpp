#pragma once

#include <stdexcept>
#include <string>

#include <mh/source_location.hpp>

namespace mh
{
	class not_implemented_error : public std::logic_error
	{
		static std::string format_message(const mh::source_location& location)
		{
			std::string retVal;
			retVal
				.append(location.file_name())
				.append("(")
				.append(std::to_string(+location.line()))
				.append("):")
				.append(location.function_name())
				.append(" not implemented");

			return retVal;
		}

	public:
		explicit not_implemented_error(MH_SOURCE_LOCATION_AUTO(location)) :
			std::logic_error(format_message(location)),
			m_Location(location)
		{
		}

		const mh::source_location& location() const noexcept { return m_Location; }

	private:
		mh::source_location m_Location;
	};
}
