#include "SteamAPI.h"
#include "SteamHistoryAPI.h"


#include "HTTPHelpers.h"

#include <nlohmann/json.hpp>

/// <summary>
/// gets sourcebans from XVF's steamhistory site.
///
/// largely copypasted from SteamAPI::GetPlayerSummariesAsync
/// </summary>
/// <param name="apiKey"></param>
/// <param name="steamIDs"></param>
/// <param name="client"></param>
/// <returns></returns>
mh::task<tf2_bot_detector::SteamHistoryAPI::PlayerSourceBansResponse>
	tf2_bot_detector::SteamHistoryAPI::GetPlayerSourceBansAsync(
		const std::string& apiKey,
		const std::vector<SteamID>& steamIDs,
		const HTTPClient& client
	)
{
	if (steamIDs.empty())
		co_return{};
	
	std::string requestSteamIDs = tf2_bot_detector::SteamAPI::GenerateSteamIDsQueryParam(steamIDs, 100);
	requestSteamIDs.at(0) = '&';

	// copied segments of GenerateSteamAPIURL; consolidate later?
	URL requestURL = URL(mh::format(MH_FMT_STRING("https://steamhistory.net/api/sourcebans?shouldkey=1&key={}{}"), apiKey, requestSteamIDs));

	auto clientPtr = client.shared_from_this();
	const std::string data = co_await clientPtr->GetStringAsync(requestURL);

	nlohmann::json json;
	try
	{
		json = nlohmann::json::parse(data);
	}
	catch (...)
	{
		// TODO
	}

	co_return json.at("response").get<PlayerSourceBansResponse>();
}


void tf2_bot_detector::SteamHistoryAPI::from_json(const nlohmann::json& j, BanState& d) {
	if (j == "Permanent") {
		d = BanState::Permanent;
	}
	else if (j == "Temp-Ban") {
		d = BanState::Current;
	}
	else if (j == "Unbanned") {
		d = BanState::Unbanned;
	}
	else {
		d = BanState::Expired;
	}
}

void tf2_bot_detector::SteamHistoryAPI::from_json(const nlohmann::json& j, PlayerSourceBan& d) {
	d = {};

	d.m_ID = j.at("SteamID");

	// HOW CAN NAME BE NULL WTF
	if (j.at("Name").is_string()) {
		d.m_UserName = j.at("Name");
	}

	d.m_BanState = j.at("CurrentState").get<BanState>();
	
	if (j.at("BanReason").is_string()) {
		d.m_BanReason = j.at("BanReason");
	}

	if (j.at("UnbanReason").is_string()) {
		d.m_UnbanReason = j.at("UnbanReason");
	}

	// it's epoch time.... in string format.
	if (auto found = j.find("BanTimestamp"); found != j.end()) {
		d.m_BanTimestamp = std::chrono::system_clock::time_point(std::chrono::seconds(found->get<std::int64_t>()));
	}

	if (auto found = j.find("UnbanTimestamp"); found != j.end()) {
		d.m_UnbanTimestamp = std::chrono::system_clock::time_point(std::chrono::seconds(found->get<std::int64_t>()));
	}

	d.m_Server = j.at("Server");
}


void tf2_bot_detector::SteamHistoryAPI::from_json(const nlohmann::json& j, PlayerSourceBansResponse& d) {
	d = {};

	for (auto iter = j.items().begin(); iter != j.items().end(); iter++) {
		PlayerSourceBans bans = iter.value();
		d.insert(std::make_pair(SteamID(iter.key()), bans));
	}
}
