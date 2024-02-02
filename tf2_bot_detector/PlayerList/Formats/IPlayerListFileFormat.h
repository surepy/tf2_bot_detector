#pragma once
#include <coroutine>
#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

#include "Playerlist/PlayerAttribute.h"

namespace tf2_bot_detector::playerlist::formats {
	/// <summary>
	/// interface class for how file formats should be defined.
	/// </summary>
	class IPlayerListFileFormat {
	protected:
		std::string m_Title;
		std::string m_Description;

		/// <summary>
		/// a list of steamids we should use for saving. 
		/// </summary>
		std::unordered_set<SteamID> m_SavedIDs;
	public:
		void addID(const SteamID& id) { m_SavedIDs.insert(id); }
		void removeID(const SteamID& id) { m_SavedIDs.erase(id); }
		const std::unordered_set<SteamID>& getIDs() const { return m_SavedIDs; }
		std::string getTitle() const { return m_Title; }
		std::string getDescription() const { return m_Description; }
		virtual bool isRemote() = 0;

		virtual bool attemptLoad(std::vector<PlayerEntry>& out) = 0;
		/// <summary>
		/// looks like correct schema, we can attempt to call Load().
		/// </summary>
		/// <returns></returns>
		virtual bool valid() = 0;

		virtual bool save(const std::vector<PlayerEntry>& in) = 0;
	};
}
