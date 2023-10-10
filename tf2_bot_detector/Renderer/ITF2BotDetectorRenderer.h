/// <summary>
/// Goal: have an imgui renderer base that is compatible with 2 configurations.
///
/// - dx9 (overlay hook)
/// - sdl2 opengl (external window)
///
/// implementation slightly copied from Pazer's imgui_desktop.
///
/// TOOD: move to launcher configuration; major project restructure required
///
/// restructure plans:
/// tf2_bot_detector - outputs object library for building two configurations below
/// tf2_bot_detector_overlay - outputs tf2_bot_detector_overlay.dll which you inject to tf2
/// tf2_bot_detector_external - outputs tf2_bot_detector.dll same as current
/// tf2_bot_detector_launcher - for launching ^
/// 
/// </summary>
class ITF2BotDetectorRenderer {
	/// <summary>
	/// Update the application state.
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// ImGui::Render() related functions.
	/// </summary>
	virtual void DrawFrame() = 0;

	/// <summary>
	/// Should we stop running and destroy?
	/// </summary>
	/// <returns></returns>
	virtual bool ShouldQuit() = 0;

	/// <summary>
	/// how frequent should DrawFrame run?
	///
	/// note: ignored in hook mode.
	/// </summary>
	/// <param name="frameTime">frame time in ms</param>
	virtual void SetFramerate(float) = 0;
	virtual float GetFramerate() = 0;
};
