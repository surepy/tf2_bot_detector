#pragma once
#include "SteamID.h"

namespace tf2_bot_detector
{
	/// <summary>
	/// better PlayerAttribute (configurable)
	/// Default: Cheater / Suspicious / Exploiter / Racist (Backwards compat.)
	/// </summary>
	class PlayerAttribute2 {
		/// <summary>
		/// name of attribute
		/// </summary>
		std::string m_Name;

		/// <summary>
		/// this attribute should auto-kick.
		/// </summary>
		bool m_ShouldAutoKick = false;

		// m_DefaultColor = something
	};

	/// <summary>
	/// Where is this PlayerList data coming from?
	/// </summary>
	enum class PlayerListSource
	{
		SQLDB,
		JSON,
	};

	/// <summary>
	/// A PlayerList, loaded from sources.
	///
	/// sources can be:
	///  1. JSON (version 3)
	///  2. sqlite database.
	/// </summary>
	class PlayerList {


	};



};
