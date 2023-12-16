#pragma once
#include <functional>
#include <string>

/// <summary>
/// Goal: have an imgui renderer base that is compatible with 2 configurations.
///
/// - dx9 (overlay hook)
/// - sdl2 opengl3 (external window)
///
/// implementation slightly (blatantly) copied from Pazer's imgui_desktop.
///
/// TOOD: move to launcher configuration; major project restructure required
///
/// restructure plans:
/// tf2_bot_detector - outputs object library for building two launchable configurations below
/// tf2_bot_detector_overlay - outputs tf2_bot_detector_overlay.dll which you inject to tf2
/// tf2_bot_detector_external - outputs tf2_bot_detector.dll same as current
/// tf2_bot_detector_launcher - for launching ^
///
/// (new plan)
/// or i can have it so that tf2_bot_detector has both overlay and external methods built in
/// (overlay will be called by dllmain, external with laucher exported function)
/// idk we'll see how it goes
///
/// </summary>
class TF2BotDetectorRendererBase {
public:
	TF2BotDetectorRendererBase() {
		ptr = this;
	}
	~TF2BotDetectorRendererBase() {
		ptr = nullptr;
	}

	/// <summary>
	/// ImGui::Render() related functions.
	/// </summary>
	virtual void DrawFrame() = 0;

	/// <summary>
	/// registerable draw() function.
	/// </summary>
	typedef std::function<void()> DrawableCallbackFn;

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	virtual std::size_t RegisterDrawCallback(DrawableCallbackFn) = 0;

	/// <summary>
	/// Should we stop running and destroy?
	/// </summary>
	/// <returns></returns>
	virtual bool ShouldQuit() const = 0;
	
	/// <summary>
	/// how frequent should DrawFrame run?
	///
	/// note: ignored in overlay mode.
	/// </summary>
	/// <param name="frameTime">frame time in ms</param>
	virtual void SetFramerate(float) = 0;
	virtual float GetFramerate() const = 0;

	/// <summary>
	/// is this "window" in focus?
	///
	/// note: ignored in overlay mode.
	/// </summary>
	/// <returns></returns>
	virtual bool InFocus() const = 0;

	/// <summary>
	/// is this "window" in focus?
	///
	/// note: ignored in overlay mode.
	/// </summary>
	/// <returns></returns>
	virtual std::string RendererInfo() const = 0;


	// do we even need these features?
	inline static TF2BotDetectorRendererBase* ptr;
	static TF2BotDetectorRendererBase* GetRenderer() { return ptr; }
};
