#pragma once

#include "ConfigHelpers.h"
#include "ModeratorLogic.h"
#include "SteamID.h"

#include <mh/coroutine/generator.hpp>
#include <nlohmann/json_fwd.hpp>

#include <bitset>
#include <chrono>
#include <filesystem>
#include <map>
#include <optional>

/// <summary>
/// A PlayerList, loaded from veriety of sources.
/// </summary>
namespace tf2_bot_detector
{
	/// <summary>
	/// better PlayerAttribute (configurable)
	/// </summary>
	class PlayerAttribute2 {
		std::string m_Name;
		bool m_ShouldAutoKick = false;
	};


	class PlayerList {

	};



};
