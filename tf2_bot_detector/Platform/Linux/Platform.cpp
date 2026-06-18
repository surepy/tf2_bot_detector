#include "Platform/Platform.h"
#include "Log.h"

#include <mh/memory/unique_object.hpp>
#include <mh/text/fmtstr.hpp>

#include <mh/io/filesystem_helpers.hpp>

#include <cstdlib>
#include <fstream>

// "ERROR_PRIVILEGE_NOT_HELD" doesn't really seem to apply in linux, so just do permission denied again (lol)
const std::error_code tf2_bot_detector::Platform::ErrorCodes::PRIVILEGE_NOT_HELD(static_cast<int>(std::errc::permission_denied), std::system_category());

tf2_bot_detector::Platform::OS tf2_bot_detector::Platform::GetOS()
{
	return OS::Linux;
}

bool tf2_bot_detector::Platform::IsDebuggerAttached()
{
    // lol, may just not fix (no point, not used anywhere)
	return false;
}

bool tf2_bot_detector::Platform::IsPortAvailable(uint16_t port)
{
    // convert port number into hex
    std::string_view port_str = fmt::format("{0:04X}", port);

    // entries of /proc/net/tcp
    // we do not need to check tcp6 because tf2??? ipv6????
    std::string entry;

    std::ifstream open_ports("/proc/net/tcp", std::ios::in);

    // sl  local_address (...)
    std::getline(open_ports, entry);

    while (std::getline(open_ports, entry)) {
        //   0: 00000000:699C 
        //      ^
        size_t localaddr_loc = entry.find(":") + 2;
        //   0: 00000000:699C 
        //               ^
        std::string port = entry.substr(localaddr_loc + 9, 4);

        if (port == port_str) {
            return false;
        }
    }

    return true;
}

std::filesystem::path tf2_bot_detector::Platform::GetCurrentExeDir()
{
    // When run as an AppImage, /proc/self/exe points inside the read-only squashfs mount, but
    // cfg/fonts/logs/temp/etc. ship next to the .AppImage file (like the Windows portable layout,
    // not packaged into the image). $APPIMAGE is the absolute path to that file, so its parent is
    // the writable portable folder we want to anchor reads and writes to.
    if (const char* appimage = getenv("APPIMAGE"))
        return std::filesystem::path(appimage).parent_path();

    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);

    // readlink does not null-terminate; bound the string by the byte count it actually wrote.
    return std::filesystem::path(std::string(result, count > 0 ? count : 0)).parent_path();
}