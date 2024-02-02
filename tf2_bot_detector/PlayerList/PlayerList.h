#pragma once

#include "ModeratorLogic.h"
#include "SteamID.h"

#include <mh/coroutine/generator.hpp>
#include <nlohmann/json_fwd.hpp>

#include <bitset>
#include <chrono>
#include <filesystem>
#include <map>
#include <optional>

#include "PlayerList/PlayerAttribute.h"
#include "PlayerList/Formats/IPlayerListFileFormat.h"

/// <summary>
/// A PlayerList, loaded from variety of sources.
/// </summary>
namespace tf2_bot_detector
{
	/// <summary>
	/// Main playerlist, combining all other playerlist sources
	///
	/// sources can be:
	///  1. JSON (version 3)
	/// </summary>
	class PlayerList {
		/// <summary>
		/// to keep track of metadata of files that we have loaded in, handles saving operation.
		/// </summary>
		std::unordered_map<std::filesystem::path, std::shared_ptr<playerlist::formats::IPlayerListFileFormat>> m_loadedFiles;

		/// <summary>
		/// our actual loaded playerlist
		/// </summary>
		std::unordered_map<SteamID, playerlist::SavedPlayerEntries> m_Players;

		/// <summary>
		/// attributes on players which will not be saved.
		/// </summary>
		std::unordered_map<SteamID, playerlist::TransientPlayerEntries> m_TransientPlayers;

		/// <summary>
		/// loaded attributes which will stay.
		/// </summary>
		std::unordered_map<std::string, playerlist::PlayerAttribute2> m_LoadedAttributes;
	public:
		PlayerList();

		/// <summary>
		/// attempt to load every file that matches filename
		/// in cfg folder.
		/// </summary>
		/// <returns></returns>
		bool Load();

		bool LoadDB(std::filesystem::path file);

		/// <summary>
		/// test/dummy data to do
		/// </summary>
		/// <returns></returns>
		bool LoadTestEntries();

		void CreateDefaultAttributes();

		playerlist::PlayerAttribute2& GetAttribute(std::string attribute_key);
		playerlist::PlayerAttribute2& GetOrCreateAttribute(std::string attribute_key);
		playerlist::PlayerAttribute2& LoadAttribute(playerlist::PlayerAttribute2&& attribute);

		playerlist::SavedPlayerEntries GetPlayerEntries(const SteamID& playerID) { return m_Players.at(playerID); };

		// modifies default playerlist.json (for now)
		bool ModifyPlayerEntry(SteamID playerID, const std::function<bool(playerlist::PlayerEntry& data)>& func);
		bool ModifyPlayerEntry(SteamID playerID, std::filesystem::path source, const std::function<bool(playerlist::PlayerEntry& data)>& func);
		bool RemovePlayerEntry(SteamID playerID, std::filesystem::path source);
		bool SavePlayerList(const std::filesystem::path& source);


		void LoadPlayerEntry(playerlist::PlayerEntry attr);

		// probably thread-unsafe.
		size_t UnloadPlayerList(std::filesystem::path path);

		/// <summary>
		/// clear our playerlist (for reloading, etc)
		/// </summary>
		void clear() { m_Players.clear(); }

		/// <summary>
		/// player entry count
		/// </summary>
		size_t size() { return m_Players.size(); }

		const playerlist::TransientPlayerEntries& GetPlayerTransientAttributes(const SteamID& playerID);

		void AddPlayerTransientAttributes(const SteamID& playerID, playerlist::PlayerAttribute2& attr);

		/// <summary>
		/// clear a certain player's transient attributes, in case of false positives.
		///
		/// TODO: "ignore this player from rule"
		/// </summary>
		/// <param name="playerID">target player</param>
		/// <returns>entry index</returns>
		size_t ClearPlayerTransientAttributes(const SteamID& playerID) { return m_TransientPlayers.erase(playerID); }

		/// <summary>
		/// clear all transient marks
		/// </summary>
		void transientClear() { m_TransientPlayers.clear(); }

		/// <summary>
		/// player entry count
		/// </summary>
		size_t transientSize() { return m_Players.size(); }
	};
};
