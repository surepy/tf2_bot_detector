#include "Platform/Platform.h"
#include "Log.h"

#include <mh/memory/unique_object.hpp>
#include <mh/text/fmtstr.hpp>

// the fuck is this
const std::error_code tf2_bot_detector::Platform::ErrorCodes::PRIVILEGE_NOT_HELD(12345, std::system_category());


tf2_bot_detector::Platform::OS tf2_bot_detector::Platform::GetOS()
{
	return OS::Linux;
}

bool tf2_bot_detector::Platform::IsDebuggerAttached()
{
    // lol, may just not fix (no point, not used anywhere)
	return false;
}

// TODO implement
bool tf2_bot_detector::Platform::IsPortAvailable(uint16_t port)
{
    return true;
}

// TODO implement
std::filesystem::path tf2_bot_detector::Platform::GetCurrentExeDir()
{
    // get from pid code may move to processes ig
    return std::filesystem::current_path();
}