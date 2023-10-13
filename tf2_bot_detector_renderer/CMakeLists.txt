cmake_minimum_required(VERSION 3.13)

include(../cmake/init-preproject.cmake)
project(tf2_bot_detector_renderer)
include(../cmake/init-postproject.cmake)

# our source files
add_library(tf2_bot_detector_renderer OBJECT
	"d3d9.cpp"
	"d3d9.h"
	"ITF2BotDetectorRenderer.h"
	"sdl2opengl.h"
	"sdl2opengl.cpp"
)

# our imgui directory.
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/submodules/imgui")

# imgui source file / defn stuff
# I'm copying a lot of stuff from pazer's imgui_desktop.
find_package(SDL2 CONFIG REQUIRED)

# TODO: rename this to just "imgui"
add_library(min_imgui 
	"${IMGUI_DIR}/imconfig.h"
	"${IMGUI_DIR}/imgui.cpp"
	"${IMGUI_DIR}/imgui.h"
	"${IMGUI_DIR}/imgui_demo.cpp"
	"${IMGUI_DIR}/imgui_draw.cpp"
	"${IMGUI_DIR}/imgui_internal.h"
	"${IMGUI_DIR}/imgui_tables.cpp"
	"${IMGUI_DIR}/imgui_widgets.cpp"
	"${IMGUI_DIR}/imstb_rectpack.h"
	"${IMGUI_DIR}/imstb_textedit.h"
	"${IMGUI_DIR}/imstb_truetype.h"
	"${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
	"${IMGUI_DIR}/misc/cpp/imgui_stdlib.h"
)

target_sources(min_imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp")
target_sources(min_imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdl2.h")

# imgui includes
target_include_directories(min_imgui PUBLIC
	"${IMGUI_DIR}"
	"${IMGUI_DIR}/misc/cpp"
)
target_link_libraries(min_imgui PUBLIC SDL2::SDL2)
target_compile_definitions(min_imgui PUBLIC IMGUI_USE_SDL2)

# renderer includes
find_package(fmt CONFIG REQUIRED)

target_link_libraries(tf2_bot_detector_renderer
	PUBLIC
		min_imgui
		SDL2::SDL2
		fmt::fmt
		mh::stuff
)

target_include_directories(tf2_bot_detector_renderer PRIVATE "${CMAKE_SOURCE_DIR}/tf2_bot_detector")
target_include_directories(tf2_bot_detector_renderer PUBLIC "${PROJECT_SOURCE_DIR}")