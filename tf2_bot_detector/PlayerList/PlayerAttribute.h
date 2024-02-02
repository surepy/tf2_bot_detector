#pragma once
#include "SteamID.h"
#include <string>
#include <vector>
#include <set>

class PlayerList;

namespace tf2_bot_detector::playerlist
{
	/// <summary>
	/// better PlayerAttribute (configurable)
	/// Default: Cheater / Suspicious / Exploiter / Racist (Backwards compat.)
	/// </summary>
	class PlayerAttribute2 {
		/// <summary>
		/// name of attribute, works as a key (unique)
		/// </summary>
		std::string m_Name;

		/// <summary>
		/// description of attribute, if you need it.
		/// </summary>
		std::string m_Description;

		/// <summary>
		/// this attribute should auto-kick.
		///
		/// default false except "cheater" and "bot"
		/// </summary>
		bool m_ShouldAutoKick = false;

		/// <summary>
		/// this attribute should warn in chat.
		///
		/// default false except "cheater" and "bot"
		/// </summary>
		bool m_ShouldWarnAll = false;

		/// <summary>
		/// a setting for when your party literally does not know how to read attributes
		/// and just goes "i see name i call vote"
		///
		/// yes, this did happen.
		/// no, i will not tell who.
		/// </summary>
		bool m_ShouldWarnParty = false;

	public:
		PlayerAttribute2(std::string, bool, bool);
		PlayerAttribute2(std::string, std::string, bool, bool);

		const std::string& getName() { return m_Name;  }
		std::string getID() const;
		const std::string& getDescription() { return m_Description; }
		bool getShouldAutoKick() const { return m_ShouldAutoKick; }
		bool getShouldWarnAll() const { return m_ShouldWarnAll; }
		bool getShouldWarnParty() const { return m_ShouldWarnParty; }
	};

	typedef std::set<std::string> PlayerAttributes;

	/// <summary>
	/// An entry of a player
	///
	/// e n t r y - s h i m a s i t a ! (maimai) 
	/// </summary>
	class PlayerEntry {
	public:
		PlayerEntry(const SteamID& id);

		SteamID m_SteamID;
		SteamID GetSteamID() const { return m_SteamID; }

		/// <summary>
		/// are we allowed to modify this PlayerEntry?
		/// if not, false (update_url is set / is a read-only supported playerlist type)
		/// </summary>
		bool m_ModifyAble = true;

		std::filesystem::path m_File;

		/// <summary>
		/// list of attributes, required
		/// </summary>
		PlayerAttributes m_Attributes;

		struct SeenAs
		{
			std::chrono::system_clock::time_point m_Time;
			std::string m_PlayerName;
		};

		std::optional<SeenAs> m_LastSeen;
		std::optional<SeenAs> m_FirstSeen;

		std::vector<std::string> m_Proofs;
	};

	void to_json(nlohmann::json& j, const PlayerEntry& d);

	class SavedPlayerEntries {
		// functions todo
		std::unordered_map<std::filesystem::path, PlayerEntry> m_Entries;
	public:
		bool HasAttribute(const std::string& attr);
		bool HasAttribute(const playerlist::PlayerAttribute2& attr);

		void AddEntry(const PlayerEntry& entry) { m_Entries.insert({entry.m_File, entry}); }
		std::size_t EraseEntry(const std::filesystem::path& file) { return m_Entries.erase(file); }

		std::unordered_map<std::filesystem::path, PlayerEntry>& GetEntries() { return m_Entries; };

		friend class PlayerList;
	};

	typedef PlayerAttributes TransientPlayerEntries;
};
