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
#include <chrono>

#ifdef WIN32
#include "Platform/Windows/WindowsHelpers.h"
#include <Windows.h>
#include <Objbase.h>
#include <shellapi.h>

#include "d3d9.h"
#else 
// Why do they keep deprecating simple functions for complicated functions. 
// Instead of busting your brains on nanosecond() might as well use usleep for the time being. 
// – user9599745 Jun 6, 2019 at 9:02
// https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds#comment99538873_1157217
int sleep_ms(int msec) {
	struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

#define Sleep sleep_ms
#endif

#include "renderer.h"

using namespace std::string_literals;

namespace tf2_bot_detector
{
#ifdef _DEBUG
	uint32_t g_StaticRandomSeed = 0;
#endif
}

/// <summary>
/// main entry point for tf2bd/External window
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
TF2_BOT_DETECTOR_EXPORT int tf2_bot_detector::RunProgram(int argc, const char** argv)
{
#if _WIN32
	OutputDebugStringA("Hello from RunProgram (pre-log!)");
#endif
	{
		IFilesystem::Get().Init();
		ILogManager::GetInstance().Init();

		DebugLog("Hello from RunProgram!");

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
		TF2BDRenderer renderer;

		std::shared_ptr<TF2BDApplication> app = std::make_shared<TF2BDApplication>();

		app.get()->SetForwardedCommandLineArguments(forwarded_arg);
		app.get()->SetLaunchedFromSteam(running_from_steam);

		// register mainwindow
		{
			std::shared_ptr<MainWindow> mainwin = std::make_shared<MainWindow>(app.get());

			mainwin->OnImGuiInit();
			mainwin->OpenGLInit();

			// renderer.RegisterDrawCallback([]() {});
			renderer.RegisterDrawCallback([main_window = std::move(mainwin), &renderer]() {
				// important note: while mainwindow handles only drawing related stuff,
				// it also handles "wake from sleep", when our application log (not tf2 log!) has new stuff
				main_window->Draw();
			});
		}

		if (false)
		{
			std::shared_ptr<PlayerListManagementWindow> plist = std::make_shared<PlayerListManagementWindow>();

			renderer.RegisterDrawCallback([window = std::move(plist)]() {
				window->Draw();
			});
		}

		std::chrono::milliseconds last_update;
		auto now_milis = []() {
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()
			);
		};

		DebugLog("Entering event loop...");
		while (!renderer.ShouldQuit()) {
			// app is in focus or app queued update (/sleep disabled)
			if ((renderer.InFocus() || app->ShouldUpdate())) {
				app->Update();
				last_update = now_milis();
			}
			// 100ms since last update (and sleeping)
			// TODO: make this configurable?
			else if (last_update + std::chrono::milliseconds(100) < now_milis()) {
				app->Update();
				last_update = now_milis();
			}

			renderer.DrawFrame();
		}
#endif
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

		CHECK_HR(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

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

#endif
