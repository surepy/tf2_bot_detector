#include "d3d9.h"

// windows is assumed in this file.
#ifdef WIN32

#include <Windows.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <d3d9.h>
#include <d3d9helper.h>
#pragma comment(lib, "d3d9.lib")

//using namespace ImGuiDesktop;
//using namespace std::string_literals;

/// <summary>
/// create a dummy device, and populate our pointers for dx hooking.
/// 
/// yes, I _did_ paste this a lot, do you have a problem?
/// </summary>
TF2BotDetectorD3D9Renderer::TF2BotDetectorD3D9Renderer()
{
	IDirect3D9* d3d_ptr = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d_ptr) {
		// fail and die!
		OutputDebugString("fail and die 1\n");
	}

	// search for our window that we can feed into hDeviceWindow
	HWND window;

	// see WNDENUMPROC
	// we're searching for a window is our process id.
	auto enumFn = [&window](_In_ HWND hWnd) {
		DWORD proc_id;
		GetWindowThreadProcessId(hWnd, &proc_id);

		// keep searching, this isn't the window we're looking for 
		if (GetCurrentProcessId() != proc_id) {
			return TRUE;
		}

		window = hWnd;
		return FALSE;
	};

	// this is kind of awful, it uses LPARAM to bring a function pointer to enumFn,
	// and creates a 2nd lambda to run that.
	EnumWindows(
		[] (_In_ HWND hWnd, _In_ LPARAM enumFnPtr) {
			return (*reinterpret_cast<decltype(enumFn)*>(enumFnPtr))(hWnd);
		},
		reinterpret_cast<LPARAM>(&enumFn)
	);

	if (!window) {
		// window isn't found; fail and die also!
		OutputDebugString("fail and die 2\n");
	}

	D3DPRESENT_PARAMETERS present_paramenters = {
		.SwapEffect = D3DSWAPEFFECT_DISCARD,
		.hDeviceWindow = window,
		.Windowed = false,
	};

	IDirect3DDevice9* dummy_device = nullptr;

	HRESULT device_created_result = d3d_ptr->CreateDevice(
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_paramenters, &dummy_device
	);

	if (device_created_result != S_OK) {
		// we failed to make a device, try changing windowed state and try again.
		present_paramenters.Windowed = !present_paramenters.Windowed;

		device_created_result = d3d_ptr->CreateDevice(
			D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_paramenters, &dummy_device
		);
	}

	// we still failed, we should not bother anymore.
	if (device_created_result != S_OK) {
		d3d_ptr->Release();
		// fail and die!
	}

	// copy our pointers.
	memcpy_s(d3d9DevicevTable, sizeof(d3d9DevicevTable), *(void***)(dummy_device), sizeof(d3d9DevicevTable));
}

TF2BotDetectorD3D9Renderer::~TF2BotDetectorD3D9Renderer()
{
}

void TF2BotDetectorD3D9Renderer::DrawFrame()
{
}

std::size_t TF2BotDetectorD3D9Renderer::RegisterDrawCallback(DrawableCallbackFn)
{
	return std::size_t();
}

bool TF2BotDetectorD3D9Renderer::ShouldQuit() const
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
float TF2BotDetectorD3D9Renderer::GetFramerate() const
{
	return -1;
}

bool TF2BotDetectorD3D9Renderer::InFocus() const
{
	return true;
}

std::string TF2BotDetectorD3D9Renderer::RendererInfo() const
{
	return "DirectX 9";
}


#endif
