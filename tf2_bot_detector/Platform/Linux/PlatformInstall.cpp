#include "Platform/Platform.h"

using namespace tf2_bot_detector;

bool tf2_bot_detector::Platform::IsInstalled()
{
	return false;
}

bool tf2_bot_detector::Platform::CanInstallUpdate(const BuildInfo& bi)
{
	return false;
}
