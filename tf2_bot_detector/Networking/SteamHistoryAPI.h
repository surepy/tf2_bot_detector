#pragma once

#include "SteamID.h"
#include "HTTPClient.h"

#include <mh/coroutine/task.hpp>
#include <mh/reflection/enum.hpp>
#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <vector>

namespace tf2_bot_detector
{
	class IHTTPClient;
}

namespace tf2_bot_detector::SteamHistoryAPI
{
	enum BanState {
		Expired,
		Unbanned,
		Current, // "Temp-Ban"
		Permanent
	};

	enum class ErrorCode
	{
		OK,
		FeatureDisabled,
		InvalidAPIKey
	};

	struct PlayerSourceBan {
		SteamID m_ID;

		BanState m_BanState;

		/// <summary>
		/// this user was banned as this username.
		/// </summary>
		std::string m_UserName = "Unknown";

		std::string m_BanReason = "";
		std::string m_UnbanReason = "";

		time_point_t m_BanTimestamp;
		time_point_t m_UnbanTimestamp;

		std::string m_Server;
	};

	typedef std::vector<PlayerSourceBan> PlayerSourceBans;
	typedef std::unordered_map<SteamID, PlayerSourceBans> PlayerSourceBansResponse;

	typedef std::unordered_map<std::string, PlayerSourceBan> PlayerSourceBanState;

	void from_json(const nlohmann::json& j, BanState& d);
	void from_json(const nlohmann::json& j, PlayerSourceBan& d);
	void from_json(const nlohmann::json& j, PlayerSourceBansResponse& d);

	mh::task<PlayerSourceBansResponse> GetPlayerSourceBansAsync(const std::string& apiKey, const std::vector<SteamID>& steamIDs, const HTTPClient& client);
}

// for our logging functions.
MH_ENUM_REFLECT_BEGIN(tf2_bot_detector::SteamHistoryAPI::BanState)
	MH_ENUM_REFLECT_VALUE(Expired)
	MH_ENUM_REFLECT_VALUE(Unbanned)
	MH_ENUM_REFLECT_VALUE(Current)
	MH_ENUM_REFLECT_VALUE(Permanent)
MH_ENUM_REFLECT_END()
