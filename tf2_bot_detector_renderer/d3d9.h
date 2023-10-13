#include "ITF2BotDetectorRenderer.h"

/// <summary>
/// d3d9 implementation
/// </summary>
class TF2BotDetectorD3D9Renderer : public ITF2BotDetectorRenderer {
public:
	TF2BotDetectorD3D9Renderer();
	~TF2BotDetectorD3D9Renderer();

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
};


