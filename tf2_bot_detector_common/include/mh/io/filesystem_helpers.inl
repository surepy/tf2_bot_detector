#ifdef MH_COMPILE_LIBRARY
#include "filesystem_helpers.hpp"
#else
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

MH_COMPILE_LIBRARY_INLINE std::filesystem::path mh::filename_without_extension(std::filesystem::path path)
{
	return std::move(path.filename().replace_extension());
}

MH_COMPILE_LIBRARY_INLINE std::filesystem::path mh::replace_filename_keep_extension(std::filesystem::path path, std::filesystem::path newFilename)
{
	return std::move(path.replace_filename(newFilename.replace_extension(path.extension())));
}
