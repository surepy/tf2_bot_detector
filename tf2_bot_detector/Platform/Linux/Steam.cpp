#include "Platform/Platform.h"
#include "Util/TextUtils.h"
#include "Log.h"

#include "SteamID.h"

#include <vdf_parser.hpp>
#include <mh/text/string_insertion.hpp>

std::filesystem::path tf2_bot_detector::Platform::GetCurrentSteamDir()
{
	return std::filesystem::path(getenv("HOME")) / ".steam" / "steam";
}

// TODO: paste from msb people they probably figured out a detection method
// pasted from https://github.com/MegaAntiCheat/client-backend/blob/9714449f7cc1845200df5c537ca8d42d8eeb6c3d/src/settings.rs#L168-L205
// TODO: venture for a better detection method?
// FIXME: this doesn't follow the steamid folder that has been overwritten by settings, it should probably do that instead.
tf2_bot_detector::SteamID tf2_bot_detector::Platform::GetCurrentActiveSteamID()
{
	std::filesystem::path loginUsers = GetCurrentSteamDir() / "config" / "loginusers.vdf";

	SteamID returnUser;

	try
	{
		std::ifstream file;
		file.exceptions(std::ios::badbit | std::ios::failbit);
		file.open(loginUsers);
		tyti::vdf::object LoginUserRoot = tyti::vdf::read(file);

		assert(LoginUserRoot.name == "users");

		time_t last_time = 0;

		for (const auto& [steamidStr, accountEntry] : LoginUserRoot.childs) {
			time_t timestamp = std::stol(accountEntry.get()->attribs.at("Timestamp"));

			if (last_time == 0 || timestamp > last_time) {
				returnUser = SteamID(steamidStr);
				last_time = timestamp;
			}
		}
	}
	catch (...) {
		// do error stuff
	}


	return returnUser;
}
