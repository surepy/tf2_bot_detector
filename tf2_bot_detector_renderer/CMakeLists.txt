cmake_minimum_required(VERSION 3.13)

include(../cmake/init-preproject.cmake)
project(tf2_bot_detector_renderer)
include(../cmake/init-postproject.cmake)

# our source files
add_library(tf2_bot_detector_renderer OBJECT
	"d3d9.cpp"
	"d3d9.h"
	"ITF2BotDetectorRenderer.h"
	"ITF2BotDetectorDrawable.h"
	"sdl2opengl.h"
	"sdl2opengl.cpp"
)

if (WIN32)
	# because we're still on an old version of fmt, microsoft will scream at you (and it's very loud!)
	# remove me when we update fmt to 10.1.1.
	target_compile_definitions(tf2_bot_detector_renderer PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING) 
endif()

set_target_properties(tf2_bot_detector_renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(tf2_bot_detector_renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# lol
target_compile_definitions(tf2_bot_detector_renderer PRIVATE BD_VERSION="${TF2BD_VERSION_MAJOR}.${TF2BD_VERSION_MINOR}.${TF2BD_VERSION_PATCH}")

# imgui options
option(imgui_USE_OPENGL2 "Includes the OpenGL2 implementation." off)
option(imgui_USE_OPENGL3 "Includes the OpenGL3 implementation." on)
option(imgui_USE_DX9 "Includes the DirectX9 implementation." on)

option(imgui_USE_GLAD2 "Uses Glad2 implementation." on)

option(imgui_USE_GLBINDING "Uses glbinding for the OpenGL implementation." off)
option(imgui_USE_SDL2 "Uses the SDL2 implementation." on)

# our imgui directory.
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/submodules/imgui")

# imgui source file / defn stuff
# I'm copying a lot of stuff from pazer's imgui_desktop.

add_library(imgui STATIC
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

set_target_properties(imgui PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)

# imgui includes
target_include_directories(imgui PUBLIC
	"${IMGUI_DIR}"
	"${IMGUI_DIR}/misc/cpp"
)

# backend: GL2
if(imgui_USE_OPENGL2)
	target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp")
	target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_opengl2.h")

	target_compile_definitions(imgui PUBLIC "IMGUI_USE_OPENGL2")

	if (WIN32)
		target_link_libraries(imgui PRIVATE "OpenGL32.lib")
	endif()
endif()

# backend: GL3
if(imgui_USE_OPENGL3)
	target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
	target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_opengl3.h")

	target_compile_definitions(imgui PUBLIC "IMGUI_USE_OPENGL3")
endif()

# backend: dx9
if(imgui_USE_DX9)
endif()

# backend: vulkan (!)
if(imgui_USE_VULKAN)
endif()


# backend: SDL2
if(imgui_USE_SDL2)
	find_package(SDL2 CONFIG REQUIRED)
	target_link_libraries(imgui PUBLIC SDL2::SDL2)
	target_compile_definitions(imgui PUBLIC IMGUI_USE_SDL2)
	target_sources(imgui PRIVATE
		"${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
		"${IMGUI_DIR}/backends/imgui_impl_sdl2.h"
	)
elseif(imgui_USE_GLFW)
	find_package(glfw3 CONFIG REQUIRED)
	target_link_libraries(imgui PRIVATE glfw)
	target_compile_definitions(imgui PUBLIC IMGUI_USE_GLFW)
	target_sources(imgui PRIVATE
		"${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
		"${IMGUI_DIR}/backends/imgui_impl_glfw.h"
	)
endif()

# backend: glad2
find_package(glad2 REQUIRED)
target_link_libraries(imgui PRIVATE glad2::glad2)
target_compile_definitions(imgui PUBLIC IMGUI_USE_GLAD2)

# renderer includes
find_package(fmt CONFIG REQUIRED)

target_link_libraries(tf2_bot_detector_renderer
	PUBLIC
		imgui
                glad2::glad2
	PRIVATE
		SDL2::SDL2
		fmt::fmt
		mh::stuff
)

target_include_directories(tf2_bot_detector_renderer PRIVATE "${CMAKE_SOURCE_DIR}/tf2_bot_detector")
target_include_directories(tf2_bot_detector_renderer PUBLIC "${PROJECT_SOURCE_DIR}")
