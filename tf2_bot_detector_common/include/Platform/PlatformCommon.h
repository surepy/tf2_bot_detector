#pragma once

#include <source_location>

namespace tf2_bot_detector::Platform
{
	void* GetProcAddressHelper(const char* moduleName, const char* symbolName, bool isCritical = false, const ::std::source_location location = ::std::source_location::current());
}
