#pragma once

#include <source_location>
// remove me 
#include <mh/source_location.hpp>

namespace tf2_bot_detector::Platform
{
	void* GetProcAddressHelper(const char* moduleName, const char* symbolName, bool isCritical = false, const ::std::source_location location = ::std::source_location::current());
}
