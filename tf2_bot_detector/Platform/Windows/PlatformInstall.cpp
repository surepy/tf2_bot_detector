#include "Platform/Platform.h"
#include "Networking/HTTPClient.h"
#include "Networking/HTTPHelpers.h"
#include "Util/TextUtils.h"
#include "Log.h"
#include "UpdateManager.h"
#include "Version.h"

#include <mh/future.hpp>

#include <Windows.h>

using namespace tf2_bot_detector;

bool tf2_bot_detector::Platform::IsInstalled()
{
	return false;
}

bool tf2_bot_detector::Platform::CanInstallUpdate(const BuildInfo& bi)
{
	return false;
}
