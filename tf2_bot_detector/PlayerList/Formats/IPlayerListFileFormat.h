#pragma once
#include <coroutine>
#include <iostream>
#include <filesystem>
#include <vector>

#include "Playerlist/PlayerAttribute.h"

namespace tf2_bot_detector::playerlist::formats {
	/// <summary>
	/// interface class for how file formats should be defined.
	/// </summary>
	class IPlayerListFileFormat {
	protected:
		std::string m_Title;
		std::string m_Description;
	public:
		std::string getTitle() { return m_Title; }
		std::string getDescription() { return m_Description; }
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
