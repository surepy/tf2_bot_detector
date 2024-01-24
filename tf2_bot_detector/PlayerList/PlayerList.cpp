#include "PlayerList.h"
#include "Filesystem.h"
#include <regex>
#include <fmt/format.h>

#include "PlayerList/Formats/PlayerListJSONV3.h"

// — Today at 10:30 AM
// hey hear me out
// ```
#define yea true
#define nah false
// ```

tf2_bot_detector::PlayerList::PlayerList()
{
	CreateDefaultAttributes();
}

bool tf2_bot_detector::PlayerList::Load()
{
	// copied from ConfigHelpers.cpp / GetConfigFilePaths(string_view)
	constexpr std::filesystem::directory_options options =
		std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink;

	try
	{
		for (const auto& file : IFilesystem::Get().IterateDir("cfg", false, options))
		{
			const std::regex filenameRegex(
				R"regex(playerlist.*\.json)regex",
				std::regex::optimize | std::regex::icase
			);

			const auto path = file.path();
			const auto filename = path.filename().string();
			if (std::regex_match(filename.begin(), filename.end(), filenameRegex)) {
				// this will all be sync for now, no async.
				// sorry for slowdowns (its probably not that much).

				// try to load as pazer's format
				auto json_v3 = playerlist::formats::JsonV3(path);
				//if (json_v3.valid())
				{
					std::vector<playerlist::PlayerEntry> a;

					json_v3.attemptLoad(a);

					// m_loadedFiles.insert({ path, json_v3 });
					continue;
				}
				// do loading stuff here
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		LogException(MH_SOURCE_LOCATION_CURRENT(), e, "Failed loading playerlist.");
		return false;
	}

	return false;
}

bool tf2_bot_detector::PlayerList::LoadTestEntries()
{
	// it's me [U:1:452543165]

	// account used for injecting randomass dlls, should bother nobody
	m_TransientPlayers.insert({ SteamID(76561198851982472), { "cheater", "bot" }});

	// nullifiedcat (TOS banned/deleted) 76561198267167887

	return false;
}

void tf2_bot_detector::PlayerList::CreateDefaultAttributes()
{
	// tf2bd marks:

	// purpose of this program 1
	LoadAttribute(std::move(playerlist::PlayerAttribute2("cheater", "This user is cheating.", yea, yea)));
	// imposter sus among us
	LoadAttribute(std::move(playerlist::PlayerAttribute2("suspicious", "This user may be cheating.", nah, nah)));
	// average tf2 player
	LoadAttribute(std::move(playerlist::PlayerAttribute2("racist", "This user is seen using bigoted remarks.", nah, nah)));
	// or could be fuckin anything tbh LMAO
	LoadAttribute(std::move(playerlist::PlayerAttribute2("exploiter", "this user may have used exploits.", nah, nah)));

	// new default attributes

	// LoadAttribute(std::move(playerlist::PlayerAttribute2("idot fuck", "fuck this guy", yea, yea)));
	// purpose of this software 2
	LoadAttribute(std::move(playerlist::PlayerAttribute2("bot", "This account is running automation software to destroy public lobbies", yea, yea)));
	// special attribute, overrides and ignores all attributes
	//PlayerAttribute2 bot = PlayerAttribute2("ignore", "You have verified that this account is okay.", nah, nah);
}

tf2_bot_detector::playerlist::PlayerAttribute2& tf2_bot_detector::PlayerList::GetAttribute(std::string attribute_key)
{
	return m_LoadedAttributes.at(attribute_key);
}

tf2_bot_detector::playerlist::PlayerAttribute2& tf2_bot_detector::PlayerList::GetOrCreateAttribute(std::string attribute_key)
{
	if (!m_LoadedAttributes.contains(attribute_key)) {
		m_LoadedAttributes.insert({
				attribute_key,
				//
				playerlist::PlayerAttribute2(attribute_key, false, false)
		});
	}

	return m_LoadedAttributes.at(attribute_key);
}

tf2_bot_detector::playerlist::PlayerAttribute2& tf2_bot_detector::PlayerList::LoadAttribute(playerlist::PlayerAttribute2&& attribute)
{
	std::string id = attribute.getID();

	if (!m_LoadedAttributes.contains(id)) {
		m_LoadedAttributes.insert({ id, attribute });
	}

	return m_LoadedAttributes.at(id);
}

bool tf2_bot_detector::PlayerList::ModifyPlayerEntry(SteamID playerID, const std::function<bool(playerlist::PlayerEntry& data)>& func)
{
	return false;
}

bool tf2_bot_detector::PlayerList::ModifyPlayerEntry(SteamID playerID, std::filesystem::path source, const std::function<bool(playerlist::PlayerEntry& data)>& func)
{
	return false;
}

size_t tf2_bot_detector::PlayerList::UnloadPlayerList(std::filesystem::path path)
{
	size_t count = 0;
	for (auto& [id, entry] : m_Players) {
		entry.EraseEntry(path);
		++count;
	}
	return count;
}

const tf2_bot_detector::playerlist::TransientPlayerEntries& tf2_bot_detector::PlayerList::GetPlayerTransientAttributes(const SteamID& playerID)
{
	return m_TransientPlayers.at(playerID);
}

void tf2_bot_detector::PlayerList::AddPlayerTransientAttributes(const SteamID& playerID, playerlist::PlayerAttribute2& attr)
{
	if (m_TransientPlayers.contains(playerID)) {
		m_TransientPlayers.at(playerID).insert(attr.getID());
	}
	else {
		m_TransientPlayers.insert({ playerID, { attr.getID() }});
	}
}
