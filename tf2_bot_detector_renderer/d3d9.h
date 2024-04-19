#pragma once
#ifdef WIN32

#include "ITF2BotDetectorRenderer.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

/// <summary>
/// d3d9 implementation
/// </summary>
class TF2BotDetectorD3D9Renderer : public TF2BotDetectorRendererBase {
public:
	TF2BotDetectorD3D9Renderer();
	~TF2BotDetectorD3D9Renderer();

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
	/// note: ignored in hook mode.
	/// </summary>
	/// <param name="frameTime">frame time in ms</param>
	void SetFramerate(float);
	float GetFramerate() const;

	/// <summary>
	/// external-only feature.
	/// </summary>
	/// <returns></returns>
	bool InFocus() const;

	std::string RendererInfo() const;

private:
	/// <summary>
	/// vTables of our d3d9Device.
	/// </summary>
	void* d3d9DevicevTable[119];
};

#endif
