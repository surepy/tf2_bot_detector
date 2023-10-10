#include "ITF2BotDetectorRenderer.h"

#include <mh/source_location.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

struct SDL_Window;
struct ImGuiContext;
struct ImFontAtlas;

#define RENDERER EXTERNAL
namespace tf2_bot_detector {};

/// <summary>
/// sdl2 opengl implementation
/// </summary>
class TF2BotDetectorRenderer : public ITF2BotDetectorRenderer {
public:
	TF2BotDetectorRenderer();

	/// <summary>
	/// Update the application state.
	/// </summary>
	void Update();

	/// <summary>
	/// ImGui::Render() related functions.
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
	float frameTime = 33.3f;
};
