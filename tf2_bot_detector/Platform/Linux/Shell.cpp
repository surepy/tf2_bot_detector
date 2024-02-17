#include "../Platform.h"
#include "Util/TextUtils.h"
#include "Log.h"

#include <mh/text/string_insertion.hpp>
#include <fmt/format.h>

#include <filesystem>
#include <memory>


/// @brief opens anything with xdg-open
/// @warning potentially, i mean very insecure. do not put any user data in this function.
/// @param url 
void xdg_open(const char* url)
{
    system(fmt::format("xdg-open {}", url).c_str());
}

// TODO: implemet me
std::vector<std::string> tf2_bot_detector::Shell::SplitCommandLineArgs(const std::string_view& cmdline)
{
	return std::vector<std::string>();
}

std::filesystem::path tf2_bot_detector::Shell::BrowseForFolderDialog() {
    return {};
}

// selecting is not a feature in xdg-open
// cry about it
void tf2_bot_detector::Shell::ExploreToAndSelect(std::filesystem::path path) {
    xdg_open(path.c_str());
}

void tf2_bot_detector::Shell::ExploreTo(const std::filesystem::path& path) {
    xdg_open(path.c_str());
}

void tf2_bot_detector::Shell::OpenURL(const char* url) {
    xdg_open(url);
}

