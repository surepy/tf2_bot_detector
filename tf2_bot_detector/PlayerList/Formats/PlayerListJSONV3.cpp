#include "PlayerList/Formats/PlayerListJSONV3.h"
#include "Log.h"
#include "Filesystem.h"
#include "PlayerList/PlayerAttribute.h"

#include <nlohmann/json.hpp>

tf2_bot_detector::playerlist::formats::JsonV3::JsonV3(const std::filesystem::path& path) : m_File(path)
{
	// TODO: try to load schema only.
}

bool tf2_bot_detector::playerlist::formats::JsonV3::isRemote()
{
	return !m_UpdateURL.empty();
}

bool tf2_bot_detector::playerlist::formats::JsonV3::attemptLoad(std::vector<PlayerEntry>& out)
{
	// see parts of LoadFileInternalAsync
	nlohmann::json json;
	{
		Log("Loading {}...", m_File);

		std::string file;
		try
		{
			file = IFilesystem::Get().ReadFile(m_File);
		}
		catch (...)
		{
			LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to load {}", m_File);
			return false;
		}

		try
		{
			json = nlohmann::json::parse(file);

			// see parts of PlayerListFile::Deserialize
			std::vector<PlayerEntry> retval;

			for (const auto& player : json.at("players"))
			{
				const SteamID steamID = player.at("steamid");
				PlayerEntry parsed(steamID);
				player.get_to(parsed);
				retval.push_back(parsed);
			}

			out = retval;
		}
		catch (...)
		{
			LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to parse JSON from {}", m_File);
			return false;
		}
	}

	return false;
}

bool tf2_bot_detector::playerlist::formats::JsonV3::valid()
{
	return false;
}

bool tf2_bot_detector::playerlist::formats::JsonV3::save(const std::vector<PlayerEntry>& in)
{
	return false;
}
