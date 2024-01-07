#include "DLLMain.h"

#include "Application.h"
#include "Tests/Tests.h"
#include "Util/TextUtils.h"
#include "Log.h"
#include "Filesystem.h"

#include <mh/text/string_insertion.hpp>

#include "UI/MainWindow.h"
#include "UI/SettingsWindow.h"
#include "UI/PlayerListManagementWindow.h"

#ifdef WIN32
#include "Platform/Windows/WindowsHelpers.h"
#include <Windows.h>
#include <Objbase.h>
#include <shellapi.h>

#include "d3d9.h"
#endif

#include "sdl2opengl.h"

using namespace std::string_literals;

namespace tf2_bot_detector
{
#ifdef _DEBUG
	uint32_t g_StaticRandomSeed = 0;
	bool g_SkipOpenTF2Check = false;
#endif

	static void ImGuiDesktopLogFunc(const std::string_view& msg, const mh::source_location& location)
	{
		DebugLog(location, "[ImGuiDesktop] {}", msg);
	}
}

/// <summary>
/// main entry point for tf2bd/External window
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
TF2_BOT_DETECTOR_EXPORT int tf2_bot_detector::RunProgram(int argc, const char** argv)
{
	DebugLog("Hello from RunProgram!");
	{
		IFilesystem::Get().Init();
		ILogManager::GetInstance().Init();

		std::string forwarded_arg;
		bool running_from_steam = false;

		for (int i = 1; i < argc; i++)
		{
			DebugLog(" command-line argv[{}]: {}", i, argv[i]);
			// steam automatically injects these command line arguments.
			if (!strcmp(argv[i], "-forward") || !strcmp(argv[i], "-game") || !strcmp(argv[i], "-steam")) {
				running_from_steam = true;
			}

			if (!strcmp(argv[i], "-forward") && (i + 1) < argc) {
				forwarded_arg = argv[i + 1];
			}
#ifdef _DEBUG
			if (!strcmp(argv[i], "--static-seed") && (i + 1) < argc)
				tf2_bot_detector::g_StaticRandomSeed = atoi(argv[i + 1]);
			else if (!strcmp(argv[i], "--allow-open-tf2"))
				tf2_bot_detector::g_SkipOpenTF2Check = true;
			else if (!strcmp(argv[i], "--run-tests"))
			{
#ifdef TF2BD_ENABLE_TESTS
				return tf2_bot_detector::RunTests();
#else
				LogError("--run-tests was on the command line, but tests were not compiled in");
#endif
			}
#endif
		}

		if (running_from_steam) {
			DebugLog("Detected that we launched from Steam, using -forward if it exists.");
		}

#if defined(_DEBUG) && defined(TF2BD_ENABLE_TESTS)
		// Always run the tests debug builds (but don't quit afterwards)
		tf2_bot_detector::RunTests();
#endif


#ifndef TF2BD_OVERLAY_BUILD
		DebugLog("Initializing TF2BDApplication...");
		TF2BotDetectorSDLRenderer renderer;

		std::shared_ptr<TF2BDApplication> app = std::make_shared<TF2BDApplication>();

		app.get()->SetForwardedCommandLineArguments(forwarded_arg);
		app.get()->SetLaunchedFromSteam(running_from_steam);

		// register mainwindow
		{
			std::shared_ptr<MainWindow> mainwin = std::make_shared<MainWindow>(app.get());

			mainwin->OnImGuiInit();
			mainwin->OpenGLInit();

			// renderer.RegisterDrawCallback([]() {});
			renderer.RegisterDrawCallback([main_window = std::move(mainwin), &renderer, app]() {
				// TODO: Put this in a different thread?
				// update our main state instantly, if we are focused or we're forced by application log
				if ((renderer.InFocus() || app->ShouldUpdate())) {
					app->Update();
				}
				// update our main state, after we wait 100ms anyway (FIXME/HACK: this replicates the "FIXME" behaivor in imgui_desktop).
				// https://github.com/PazerOP/imgui_desktop/blob/2e103fd66a39725a75fe93267bff1c701f246c34/imgui_desktop/src/Application.cpp#L52-L53
				else {
					Sleep(100);
					app->Update();
				}

				// important note: while mainwindow handles only drawing related stuff,
				// it also handles "wake from sleep", when our application log (not tf2 log!) has new stuff
				main_window->Draw();
			});
		}


		{
			std::shared_ptr<PlayerListManagementWindow> plist = std::make_shared<PlayerListManagementWindow>();

			renderer.RegisterDrawCallback([window = std::move(plist)]() {
				window->Draw();
			});
		}

		DebugLog("Entering event loop...");
		while (!renderer.ShouldQuit()) {
			renderer.DrawFrame();
		}
#endif

		// this was used for "PrintLogMsg" in imgui_desktop, i'm leaving it out because
		// 1, we're launching in 1 and only 1 possible opengl configuration which is GL 4.3 + GLSL 430
		//  realistically *nobody* will need before gl 3 because even the GeForce 400 series supports it.
		// 2. lazy
		//ImGuiDesktop::SetLogFunction(&tf2_bot_detector::ImGuiDesktopLogFunc);
	}

	ILogManager::GetInstance().CleanupEmptyLogs();

	DebugLog("Graceful shutdown");
	return 0;
}

#ifdef WIN32
/// <summary>
/// workaround so pazer's SmartScreen-signed exe works, calls the other windows-specific RunProgram().
/// </summary>
/// <param name="hInstance">unused</param>
/// <param name="hPrevInstance">unused</param>
/// <param name="pCmdLine">unused</param>
/// <param name="nCmdShow">unused</param>
/// <returns></returns>
TF2_BOT_DETECTOR_EXPORT int tf2_bot_detector::RunProgram(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	return tf2_bot_detector::RunProgram();
}

/// <summary>
/// entry for tf2bd/External window, handles command line arguments for windows platform.
/// </summary>
/// <returns></returns>
TF2_BOT_DETECTOR_EXPORT int tf2_bot_detector::RunProgram()
{
	int argc;
	auto argvw = CommandLineToArgvW(GetCommandLineW(), &argc);

	std::vector<std::string> argvStrings;
	std::vector<const char*> argv;
	argvStrings.reserve(argc);
	argv.reserve(argc);

	for (int i = 0; i < argc; i++)
	{
		argvStrings.push_back(tf2_bot_detector::ToMB(argvw[i]));
		argv.push_back(argvStrings.back().c_str());
	}

	try
	{
		const auto langID = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);

		//if (!SetThreadLocale(langID))
		//	throw std::runtime_error("Failed to SetThreadLocale()");
		if (SetThreadUILanguage(langID) != langID)
			throw std::runtime_error("Failed to SetThreadUILanguage()");
		//if (ULONG langs = 1; !SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, L"pl-PL\0", &langs))
		//	throw std::runtime_error("Failed to SetThreadPreferredUILanguages()");

		const auto err = std::error_code(10035, std::system_category());
		const auto errMsg = err.message();

		CHECK_HR(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

		CHECK_HR(CoInitializeSecurity(NULL,
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities
			NULL)                        // Reserved
		);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Initialization failed", MB_OK);
		return 1;
	}

	return RunProgram(argc, argv.data());
}


#ifdef TF2BD_OVERLAY_BUILD
/// <summary>
/// Run program into an overlay mode (directx/gl/vk* - endscene)
///
/// uses dummy device method to hook d3d9.
///
/// TODO: if we ever make a linux build or whatever, we should probably consider that too.
/// </summary>
/// <param name="ignored"></param>
void tf2_bot_detector::RunProgramOverlay(HMODULE module) {
	// game isn't running
	while (FindWindowA("Valve001", 0) == 0)
		Sleep(100);

	DebugLog("Initializing TF2BDApplication (Overlay!)...");
	TF2BotDetectorD3D9Renderer* renderer = new TF2BotDetectorD3D9Renderer();

	std::shared_ptr<TF2BDApplication> app = std::make_shared<TF2BDApplication>();

	MainWindow* mainwin = new MainWindow(app.get());

	// register our draw function to endscene hook draw callback
	renderer->RegisterDrawCallback([mainwin]() { mainwin->Draw(); });

	// do tf2bd logic here
	// TODO: a way to quit
	while (true) {
		if (app->ShouldUpdate()) {
			app->Update();
		}
	}

	/*
	// client/engine isnt running
	while (GetModuleHandleA("client.dll") == 0 || GetModuleHandleA("engine.dll") == 0)
		Sleep(100);
	*/

	// clean up and exit
	FreeLibraryAndExitThread(module, 0);
}

/// <summary>
/// entry for tf2bd/overlay 
/// </summary>
/// <param name="hinstDLL"></param>
/// <param name="fdwReason"></param>
/// <param name="lpReserved"></param>
/// <returns></returns>
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module.
	DWORD fdwReason,     // reason for calling function.
	LPVOID lpReserved)  // reserved.
{
	// Perform actions based on the reason for calling.
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
#ifdef TF2BD_OVERLAY_BUILD
		MessageBoxA(NULL, "WARN: This launch configuration is potentially VAC insecure, you have been warned.", "Injected!", MB_OK | MB_ICONEXCLAMATION);
		CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)tf2_bot_detector::RunProgramOverlay, nullptr, NULL, nullptr);
#endif // TF2BD_OVERLAY_BUILD
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
#endif

#endif
