#pragma once

#include <filesystem>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh
{
	MH_STUFF_API std::filesystem::path filename_without_extension(std::filesystem::path path);
	MH_STUFF_API std::filesystem::path replace_filename_keep_extension(std::filesystem::path path, std::filesystem::path newFilename);
}

#ifndef MH_COMPILE_LIBRARY
#include "filesystem_helpers.inl"
#endif
