#include "sdl2opengl.h"

#if WIN32
#include <Windows.h>
#endif
//

#include <glad/gl.h>
//#include <gl/GL.h>
// TODO use glad

#include <imgui.h>

#include <backends/imgui_impl_sdl2.h>
#include <SDL.h>

#include <backends/imgui_impl_opengl3.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <fmt/format.h>
#include <fmt/compile.h>

#ifdef __WINRT__
#endif

#define GL_APIENTRY GLAD_API_PTR

using namespace std::string_literals;

static std::string version_string = fmt::format(FMT_COMPILE("TF2 Bot Detector v{} (sleepybuild/External)"), BD_VERSION);

TF2BotDetectorSDLRenderer::TF2BotDetectorSDLRenderer() : TF2BotDetectorRendererBase()
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		// TODO: crash
	}

	// ~~GL 3.0 + GLSL 130~~
	// * GL 4.3 + GLSL 430 core *
	const char* glsl_version = "#version 430 core";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// Create our window
	// 
	// SDL_WINDOW_HIDDEN: Why? Why do I have to do this or WinRT complains constantly about IID_ITfUIElementSink not being registered, why???
	// why do i have to create it with SDL_WINDOW_HIDDEN and then call SDL_ShowWindow at the bottom of this constructor??
	// it's also how pazer's imgui_desktop handled it;; but, but why??
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
	window = SDL_CreateWindow(version_string.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, window_flags);
	gl_context = SDL_GL_CreateContext(window);

#ifdef IMGUI_USE_GLAD2
	gladLoadGL([](const char* name) __declspec(noinline)
	{
		return reinterpret_cast<GLADapiproc>(SDL_GL_GetProcAddress(name));
	});
#endif

	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// https://github.com/ocornut/imgui/issues/2117
	// multi window causes problems in linux, apparently; investigate?
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	SDL_ShowWindow(window);
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

	// draw our registered draw functions
	{
		for (const auto& callOnDraw : drawFunctions) {
			callOnDraw();
		}
	}

	static bool so_true = true;
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Renderer Settings");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &so_true);      // Edit bools storing our window open/close state

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

		ImGui::SliderFloat("Set fps", &frameTime, 0.0f, 66.6f);

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	ImGui::ShowDemoWindow(&so_true);

	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}

	SDL_GL_SwapWindow(window);

	// lmfao
	SDL_Delay(static_cast<Uint32>(this->frameTime));
}

std::size_t TF2BotDetectorSDLRenderer::RegisterDrawCallback(DrawableCallbackFn function)
{
	drawFunctions.push_back(function);
	return drawFunctions.size();
}

bool TF2BotDetectorSDLRenderer::ShouldQuit() const
{
	return !running;
}

/// <summary>
/// sets frame time, so we can limit how much we render (60fps is a good target)
/// </summary>
/// <param name="newFrameTime"></param>
void TF2BotDetectorSDLRenderer::SetFramerate(float newFrameTime)
{ 
	this->frameTime = newFrameTime;
}

float TF2BotDetectorSDLRenderer::GetFramerate() const
{
	return this->frameTime;
}

bool TF2BotDetectorSDLRenderer::InFocus() const
{
	return SDL_GetWindowFlags(window) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS);
}

std::string TF2BotDetectorSDLRenderer::RendererInfo() const
{
	return "TF2BotDetectorSDLRenderer: OpenGl 4.3 + GLSL 430"; // fmt::format(FMT_COMPILE("TF2BotDetectorSDLRenderer: OpenGl GL 4.3 + GLSL 430"));
}
