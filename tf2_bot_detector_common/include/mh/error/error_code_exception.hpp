#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace mh
{
	template<typename TError = std::error_condition>
	class basic_error_code_exception : public std::system_error
	{
	public:
		using base_type = std::system_error;
		using code_type = TError;

	public:
		basic_error_code_exception(code_type code, const std::string& message = {}) :
			base_type(code.value(), code.category(), message)
		{
		}

		code_type code() const
		{
			auto baseCode = base_type::code();
			return code_type(baseCode.value(), baseCode.category());
		}
	};

	using error_condition_exception = basic_error_code_exception<>;
}
