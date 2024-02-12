#include "Platform/Platform.h"
#include "Util/TextUtils.h"
#include "Log.h"

#include "SteamID.h"

#include <mh/text/string_insertion.hpp>

// TODO implement
std::filesystem::path tf2_bot_detector::Platform::GetCurrentSteamDir()
{
	return std::filesystem::current_path();
}

// TODO: paste from msb people they probably figured out a detection method
tf2_bot_detector::SteamID tf2_bot_detector::Platform::GetCurrentActiveSteamID()
{
	return SteamID();
}
