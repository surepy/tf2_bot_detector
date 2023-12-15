#include "DLLMain.h"

#include "Application.h"
#include "Tests/Tests.h"
#include "UI/MainWindow.h"
#include "Util/TextUtils.h"
#include "Log.h"
#include "Filesystem.h"

#include <mh/text/string_insertion.hpp>

#ifdef WIN32
#include "Platform/Windows/WindowsHelpers.h"
#include <Windows.h>
#include <Objbase.h>
#include <shellapi.h>
#endif

#include "sdl2opengl.h"

#include "UI/SettingsWindow.h"

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

TF2_BOT_DETECTOR_EXPORT int tf2_bot_detector::RunProgram(int argc, const char** argv)
{
	{
		IFilesystem::Get().Init();
		ILogManager::GetInstance().Init();

		for (int i = 1; i < argc; i++)
		{
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

#if defined(_DEBUG) && defined(TF2BD_ENABLE_TESTS)
		// Always run the tests debug builds (but don't quit afterwards)
		tf2_bot_detector::RunTests();
#endif

		DebugLog("Initializing TF2BDApplication...");
		TF2BotDetectorSDLRenderer renderer;

		std::shared_ptr<TF2BDApplication> app = std::make_shared<TF2BDApplication>();
		{
			MainWindow* mainwin = new MainWindow(app.get());

			mainwin->OnImGuiInit();
			mainwin->OpenGLInit();
			
			renderer.RegisterDrawCallback([mainwin, &renderer, app] () {
				// TODO: move to TF2BDApplication for less clutter in MainWindow
				if (renderer.InFocus() || app->ShouldUpdate()) {
					//mainwin->OnUpdate();
					app->Update();
				}

				// important note: while mainwindow handles drawing related stuff,
				// it also handles "wake from sleep" so our app state actually gets updated.
				mainwin->Draw();
			});

			DebugLog("Entering event loop...");
			while (!renderer.ShouldQuit()) {
				renderer.DrawFrame();
			}
		}

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
/// workaround so SmartScreen-signed exe works
/// </summary>
/// <param name="hInstance">unused</param>
/// <param name="hPrevInstance">unused</param>
/// <param name="pCmdLine">unused</param>
/// <param name="nCmdShow">unused</param>
/// <returns></returns>
TF2_BOT_DETECTOR_EXPORT int tf2_bot_detector::RunProgram(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	return tf2_bot_detector::RunProgram();
}

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
#endif
