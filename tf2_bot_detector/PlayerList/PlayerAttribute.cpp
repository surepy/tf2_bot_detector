#include "PlayerAttribute.h"
#include <cctype>

tf2_bot_detector::playerlist::PlayerAttribute2::PlayerAttribute2(std::string name, bool should_kick, bool should_warn)
{
	PlayerAttribute2(name, "", should_kick, should_warn);
}

tf2_bot_detector::playerlist::PlayerAttribute2::PlayerAttribute2(std::string name, std::string desc, bool should_kick, bool should_warn_all) :
	m_Name(name),
	m_Description(desc),
	m_ShouldAutoKick(should_kick),
	m_ShouldWarnAll(should_warn_all)
{

}

std::string tf2_bot_detector::playerlist::PlayerAttribute2::getID() const
{
	std::string id = m_Name;

	// TODO: investigate how terribly this breaks in non-english names
	std::transform(id.begin(), id.end(), id.begin(),
		[](unsigned char c) {
			// lmao, spaces to underscore
			if (c == ' ') { return '_'; }
			// should be all lowercase
			return static_cast<char>(std::tolower(c));
		}
	);

	return id;
}

tf2_bot_detector::playerlist::PlayerEntry::PlayerEntry(const SteamID& id) : m_SteamID(id)
{
}


// json parsing
#include <nlohmann/json.hpp>
#include <chrono>
#include <string>

namespace tf2_bot_detector::playerlist {
	/*
	void to_json(nlohmann::json& j, const PlayerListData::LastSeen& d)
	{
		if (!d.m_PlayerName.empty())
			j["player_name"] = d.m_PlayerName;

		j["time"] = std::chrono::duration_cast<std::chrono::seconds>(d.m_Time.time_since_epoch()).count();
	}*/


	void to_json(nlohmann::json& j, const PlayerEntry& d)
	{
		j = nlohmann::json
		{
			{ "steamid", d.GetSteamID() },
			{ "attributes", d.m_Attributes }
		};

		//if (d.m_LastSeen)
		//	j["last_seen"] = *d.m_LastSeen;

		if (!d.m_Proofs.empty())
			j["proof"] = d.m_Proofs;
	}

	using namespace std::string_literals;
	using namespace std::string_view_literals;

	void from_json(const nlohmann::json& j, PlayerEntry& d) try
	{
		if (SteamID sid = j.at("steamid"); d.GetSteamID() != sid)
		{
			//throw std::runtime_error("Mismatch between target PlayerEntry SteamID ("s << d.GetSteamID() << ") and json SteamID (" << sid << ')');
		}

		d.m_Attributes = j.at("attributes");

		//if (auto lastSeen = j.find("last_seen"); lastSeen != j.end())
		//	lastSeen->get_to(d.m_LastSeen.emplace());

		//try_get_to_defaulted(j, d.m_Proof, "proof");
	}
	catch (...)
	{
		LogException();
		throw;
	}
};
