#pragma once
#include "ITF2BotDetectorRenderer.h"
#include <SDL.h>

#include <cstdint>
#include <functional>
#include <memory>

struct SDL_Window;
struct ImGuiContext;
struct ImFontAtlas;

class ITF2BotDetectorDrawable;

/// <summary>
/// sdl2-opengl implementation
///
/// currently external window only
/// </summary>
class TF2BotDetectorSDLRenderer : public TF2BotDetectorRendererBase {
public:
	TF2BotDetectorSDLRenderer();
	~TF2BotDetectorSDLRenderer();

	/// <summary>
	/// ImGui::Render() related functions.
	/// in overlay this would be somewhere in EndScene.
	/// </summary>
	void DrawFrame();

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	std::size_t RegisterDrawCallback(DrawableCallbackFn);

	/// <summary>
	/// Should we stop running and destroy?
	/// </summary>
	/// <returns></returns>
	bool ShouldQuit() const;

	/// <summary>
	/// Request application destruction.
	/// </summary>
	/// <returns></returns>
	void RequestQuit();

	/// <summary>
	/// how frequent should DrawFrame run?
	///
	/// note: ignored in overlay mode.
	/// </summary>
	/// <param name="frameTime">frame time in ms</param>
	void SetFramerate(float);
	float GetFramerate() const;

	/// <summary>
	/// determines if the user is actively interacting with our window. 
	/// </summary>
	/// <returns></returns>
	bool InFocus() const;

	std::string RendererInfo() const;
private:

	std::vector<DrawableCallbackFn> drawFunctions;

	SDL_Window* window;
	SDL_GLContext gl_context;

	bool running = true;
	// runs 60fps by default
	float frameTime = 16.6f;
};
