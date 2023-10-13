#include "d3d9.h"
#include <Windows.h>

//using namespace ImGuiDesktop;
//using namespace std::string_literals;

TF2BotDetectorD3D9Renderer::TF2BotDetectorD3D9Renderer()
{
}

TF2BotDetectorD3D9Renderer::~TF2BotDetectorD3D9Renderer()
{
}

void TF2BotDetectorD3D9Renderer::DrawFrame()
{
}

bool TF2BotDetectorD3D9Renderer::ShouldQuit()
{
	return false;
}

void TF2BotDetectorD3D9Renderer::SetFramerate(float newFrameTime)
{
	OutputDebugString("TF2BotDetectorD3D9Renderer::SetFramerate called - how the fuck did you get here?");
}

/// <summary>
/// we can't decide this.
/// </summary>
/// <returns></returns>
float TF2BotDetectorD3D9Renderer::GetFramerate()
{
	return -1;
}
