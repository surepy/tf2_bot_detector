cmake_minimum_required(VERSION 3.17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ADD_DEFINITIONS(-DUNICODE)
# ADD_DEFINITIONS(-D_UNICODE)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

option(TF2BD_ENABLE_DISCORD_INTEGRATION "Enable discord integration" on)
option(TF2BD_ENABLE_TESTS "Enable test compilation" off)

include(cmake/init-preproject.cmake)
	project(tf2_bot_detector)
include(cmake/init-postproject.cmake)

# apparent bug in VS 16.9.1 makes error messages unclickable because of wrong paths
if (MSVC)
	add_compile_options(/FC)
endif()

set_warning_level(3)

# The mh_stuff submodule is gone; its headers are vendored into
# tf2_bot_detector_common/include/mh (header-only). Provide an mh::stuff INTERFACE
# target so anything that still links it (e.g. SourceRCON's "if (NOT TARGET mh::stuff)")
# uses the vendored headers instead of fetching PazerOP/stuff from the internet.
find_package(fmt CONFIG REQUIRED)
add_library(mh_vendored INTERFACE)
add_library(mh::stuff ALIAS mh_vendored)
target_include_directories(mh_vendored INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tf2_bot_detector_common/include")
target_compile_definitions(mh_vendored INTERFACE "MH_STUFF_API=" "MH_COMPILE_LIBRARY_INLINE=inline")
target_compile_features(mh_vendored INTERFACE cxx_std_20)
target_link_libraries(mh_vendored INTERFACE fmt::fmt)

add_subdirectory(submodules/ValveFileVDF)
add_subdirectory(submodules/SourceRCON)
add_subdirectory(submodules/glad2)
# add_subdirectory(submodules/imgui_desktop)

add_subdirectory(tf2_bot_detector_common)
add_subdirectory(tf2_bot_detector_renderer)
add_subdirectory(tf2_bot_detector)

