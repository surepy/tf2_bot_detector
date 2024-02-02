#include "PlayerList/Formats/PlayerListJSONV3.h"
#include "Log.h"
#include "Filesystem.h"
#include "PlayerList/PlayerAttribute.h"

#include <nlohmann/json.hpp>

// this schema will never change (knocks on wood)
static constexpr std::string_view PLAYERLIST_SCHEMA_STRING = "https://raw.githubusercontent.com/PazerOP/tf2_bot_detector/master/schemas/v3/playerlist.schema.json";

tf2_bot_detector::playerlist::formats::JsonV3::JsonV3(const std::filesystem::path& path) : m_File(path)
{
	// we try to load the file and see if schema checks out.
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
		}

		try
		{
			json = nlohmann::json::parse(file); 
			m_Valid = json.at("$schema").get<std::string>() == PLAYERLIST_SCHEMA_STRING;

			// pre-parse file_info if possible
			if (json.find("file_info") != json.end()) {
				m_Title = json.at("file_info").value("title", "");
				m_Description = json.at("file_info").value("description", "");
				m_UpdateURL = json.at("file_info").value("update_url", "");
			}
		}
		catch (...)
		{
			LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to parse JSON from {}", m_File);
		}
	}
}

bool tf2_bot_detector::playerlist::formats::JsonV3::isRemote()
{
	return !m_UpdateURL.empty();
}

bool tf2_bot_detector::playerlist::formats::JsonV3::attemptLoad(std::vector<PlayerEntry>& out)
{
	// Note: see parts of LoadFileInternalAsync
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

			// TODO: move this into a func
			for (const auto& player : json.at("players"))
			{
				const SteamID steamID = player.at("steamid");
				PlayerEntry parsed_entry(steamID);

				// player.get_to(parsed_entry);

				// move this somewhere else
				static const std::set<std::string> valid_attributes = {
					"cheater", "suspicious", "exploiter", "racist"
				};

				// attributes, load only cheater/suspicious/exploiter/racist
				for (const auto& attribute : player.at("attributes").get<std::vector<std::string>>()) {
					if (valid_attributes.contains(attribute)) {
						parsed_entry.m_Attributes.insert(attribute);
					}
				}

				// parsed_entry.m_Attributes = player.at("attributes");

				// firstseen
				using clock = std::chrono::system_clock;
				using time_point = clock::time_point;
				using duration = clock::duration;
				using seconds = std::chrono::seconds;

				// we populate first_seen from last_seen, because this entry never updates.
				if (player.find("last_seen") != player.end()) {
					auto& last_seen = player.at("last_seen");

					parsed_entry.m_FirstSeen = {
						// FIXME: "time" could be null.
						clock::time_point(seconds(last_seen.at("time").get<seconds::rep>())),
						last_seen.value("player_name", "")
					};
				}

				// proof
				if (player.find("proof") != player.end())
					parsed_entry.m_Proofs = player.at("proof").get<std::vector<std::string>>();

				// etc
				parsed_entry.m_File = m_File;
				parsed_entry.m_ModifyAble = !isRemote();

				// mark in a list of ids we should save for this list.
				m_SavedIDs.insert(steamID);
				retval.push_back(parsed_entry);
			}

			out = retval;
			return true;
		}
		catch (...)
		{
			// no more stupid fucking _BACKUP files please
			// just let the file fail in this case.
			LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to parse JSON from {}", m_File);
			return false;
		}
	}

	return false;
}

bool tf2_bot_detector::playerlist::formats::JsonV3::valid()
{
	return m_Valid;
}

bool tf2_bot_detector::playerlist::formats::JsonV3::save(const std::vector<PlayerEntry>& in)
{
	// see tf2_bot_detector::ConfigFileBase::SaveFile
	nlohmann::json json;

	// insert that this is a pazer plist schema, this will never change.
	json["$schema"] = PLAYERLIST_SCHEMA_STRING;

	// fileinfo
	// see SharedConfigFileBase::Serialize & to_json
	if (m_Authors.empty())
		throw std::invalid_argument("Authors list cannot be empty");
	if (m_Title.empty())
		throw std::invalid_argument("Title cannot be empty");

	// file_info stuff
	nlohmann::json file_info = { { "authors", m_Authors }, { "title", m_Title } };

	if (!m_Description.empty())
		file_info["description"] = m_Description;
	if (!m_UpdateURL.empty())
		file_info["update_url"] = m_UpdateURL;

	json["file_info"] = file_info;

	auto& players = json["players"];
	players = json.array();

	/*
	for (const auto& pair : in)
	{
		if (pair.second.m_SavedAttributes.empty())
			continue;

		players.push_back(pair.second);
	}*/

	try
	{
		IFilesystem::Get().WriteFile(m_File, json.dump(1, '\t', true, nlohmann::detail::error_handler_t::ignore), PathUsage::WriteRoaming);
	}
	catch (...)
	{
		LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to write");
		return false;
	}

	return true;
}
