#include "sdl2opengl.h"

//#include "Application.h"
//#include "ScopeGuards.h"

#if WIN32
#include <Windows.h>
#endif
#include <gl/GL.h>

#include "imgui.h"
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <SDL.h>

// #include <glad/gl.h>
// TODO use glad
#include <SDL_opengl.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include "GameData/UserMessageType.h"

//using namespace ImGuiDesktop;
using namespace std::string_literals;

TF2BotDetectorSDLRenderer::TF2BotDetectorSDLRenderer()
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		// TODO: crash
	}

	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

TF2BotDetectorSDLRenderer::~TF2BotDetectorSDLRenderer()
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void TF2BotDetectorSDLRenderer::DrawFrame()
{
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
			running = false;
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
			running = false;
	}

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	bool so_true = true;
	ImGui::ShowDemoWindow(&so_true);

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &so_true);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &so_true);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

		ImGui::SliderFloat("set fps", &frameTime, 0.0f, 66.6f);

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);
}

std::size_t TF2BotDetectorSDLRenderer::RegisterDrawCallback(DrawableCallbackFn function)
{
	drawFunction = function;
	return std::size_t(0);
}

bool TF2BotDetectorSDLRenderer::ShouldQuit()
{
	return !running;
}

void TF2BotDetectorSDLRenderer::testa()
{
	drawFunction();
}

/// <summary>
/// sets frame time, so we can limit how much we render (30fps is a good target)
///
/// TODO: implement.
/// </summary>
/// <param name="newFrameTime"></param>
void TF2BotDetectorSDLRenderer::SetFramerate(float newFrameTime)
{ 
	//tf2_bot_detector::UserMessageType::ItemPickup;
	this->frameTime = newFrameTime;
}

float TF2BotDetectorSDLRenderer::GetFramerate()
{
	return this->frameTime;
}
