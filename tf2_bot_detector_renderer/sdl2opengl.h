#include "ITF2BotDetectorRenderer.h"

#include <SDL.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

struct SDL_Window;
struct ImGuiContext;
struct ImFontAtlas;

namespace tf2_bot_detector {};

/// <summary>
/// sdl2-opengl implementation
/// </summary>
class TF2BotDetectorSDLRenderer : public ITF2BotDetectorRenderer {
public:
	TF2BotDetectorSDLRenderer();
	~TF2BotDetectorSDLRenderer();

	/// <summary>
	/// ImGui::Render() related functions.
	/// in overlay this would be somewhere in EndScene.
	/// </summary>
	void DrawFrame();

	/// <summary>
	/// Should we stop running and destroy?
	/// </summary>
	/// <returns></returns>
	bool ShouldQuit();

	/// <summary>
	/// how frequent should DrawFrame run?
	///
	/// note: ignored in hook mode.
	/// </summary>
	/// <param name="frameTime">frame time in ms</param>
	void SetFramerate(float);
	float GetFramerate();

private:
	SDL_Window* window;
	SDL_GLContext gl_context;
	bool running = true;
	float frameTime = 33.3f;
};
