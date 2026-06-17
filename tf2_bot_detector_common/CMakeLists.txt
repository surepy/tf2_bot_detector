cmake_minimum_required(VERSION 3.17)

include(../cmake/init-preproject.cmake)
	project(tf2_bot_detector_common)
include(../cmake/init-postproject.cmake)

add_library(tf2_bot_detector_common STATIC)
add_library(tf2_bot_detector::common ALIAS tf2_bot_detector_common)

target_sources(tf2_bot_detector_common PRIVATE
	"include/ReleaseChannel.h"
	"include/Platform/PlatformCommon.h"
)

if (WIN32)
	target_sources(tf2_bot_detector_common PRIVATE
		"src/Platform/Windows/PlatformCommon.cpp"
	)

	# because we're still on an old version of fmt, microsoft will scream at you (and it's very loud!)
	# remove me when we update fmt to 10.1.1.
	target_compile_definitions(tf2_bot_detector_common PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
endif()

target_include_directories(tf2_bot_detector_common PUBLIC "include")

# Vendored mh utility headers (formerly the mh_stuff submodule) live under include/mh.
# They are header-only here, so provide the macros the headers expect in non-compiled-library mode.
target_compile_definitions(tf2_bot_detector_common PUBLIC
	"MH_STUFF_API="
	"MH_COMPILE_LIBRARY_INLINE=inline"
)

find_package(fmt CONFIG REQUIRED)

target_link_libraries(tf2_bot_detector_common
	PUBLIC
		fmt::fmt
)
