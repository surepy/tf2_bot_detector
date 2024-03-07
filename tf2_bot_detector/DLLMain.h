#pragma once

#ifdef WIN32
#include "tf2_bot_detector_export.h"

#include <Windows.h>
#else 
#define TF2_BOT_DETECTOR_EXPORT
#endif

namespace tf2_bot_detector
{
	TF2_BOT_DETECTOR_EXPORT int RunProgram(int argc, const char** argv);

#ifdef WIN32
	TF2_BOT_DETECTOR_EXPORT int RunProgram(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow);
	TF2_BOT_DETECTOR_EXPORT int RunProgram();

#ifdef TF2BD_OVERLAY_BUILD
	void RunProgramOverlay(HMODULE module);
#endif
#endif
}
