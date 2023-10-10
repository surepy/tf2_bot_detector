#include "sdl2opengl.h"

#include "Application.h"
#include "ScopeGuards.h"

#ifdef IMGUI_USE_GLBINDING
#include <glbinding/glbinding.h>
#include <glbinding/gl/extension.h>
#include <glbinding/gl33core/gl.h>
#include <glbinding/gl43core/gl.h>
#include <glbinding/gl20ext/gl.h>
#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/types_to_string.h>
using namespace gl33core;
#elif IMGUI_USE_GLAD2
#include <glad/gl.h>
#else
#ifdef WIN32
#include <Windows.h>
#endif
#include <gl/GL.h>
#endif

#include <imgui.h>
#include <mh/math/interpolation.hpp>
#include <mh/text/format.hpp>

/*
#ifdef IMGUI_USE_SDL2
#include <backends/imgui_impl_sdl.h>
#include <SDL.h>
#endif

#ifdef IMGUI_USE_OPENGL3
#include <backends/imgui_impl_opengl3.h>
#endif
#ifdef IMGUI_USE_OPENGL2
#include <backends/imgui_impl_opengl2.h>
#endif
*/

#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace ImGuiDesktop;
using namespace std::string_literals;

TF2BotDetectorRenderer::TF2BotDetectorRenderer()
{
}

void TF2BotDetectorRenderer::Update()
{
}

void TF2BotDetectorRenderer::DrawFrame()
{
}

bool TF2BotDetectorRenderer::ShouldQuit()
{
	return false;
}

void TF2BotDetectorRenderer::SetFramerate(float newFrameTime)
{
	this->frameTime = newFrameTime;
}

float TF2BotDetectorRenderer::GetFramerate()
{
	return 0;
}
