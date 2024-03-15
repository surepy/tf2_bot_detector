#include "WorldState.h"
#include "BatchedAction.h"
#include "GenericErrors.h"
#include "Log.h"
#include "WorldEventListener.h"
#include "GlobalDispatcher.h"
#include "Application.h"
#include "Actions/Actions.h"

#include "Config/Settings.h"
#include "Config/AccountAges.h"

#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"

#include "Networking/HTTPHelpers.h"
#include "Networking/SteamAPI.h"
#include "Networking/SteamHistoryAPI.h"
#include "Networking/LogsTFAPI.h"

#include "Util/RegexUtils.h"
#include "Util/TextUtils.h"

#include "DB/TempDB.h"

#include "GameData/IPlayer.h"
#include "GameData/Player.h"
#include "GameData/TFClassType.h"
#include "GameData/UserMessageType.h"

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace tf2_bot_detector;


Player::Player(WorldState& world, SteamID id) :
	m_World(&world)
{
	m_Status.m_SteamID = id;
}

const WorldState& Player::GetWorld() const
{
	assert(m_World);
	return *m_World;
}

const LobbyMember* Player::GetLobbyMember() const
{
	const auto steamID = GetSteamID();
	for (const auto& member : m_World->GetCurrentLobbyMembers())
	{
		if (member.m_SteamID == steamID)
			return &member;
	}
	for (const auto& member : m_World->GetPendingLobbyMembers())
	{
		if (member.m_SteamID == steamID)
			return &member;
	}

	return nullptr;
}

std::optional<UserID_t> Player::GetUserID() const
{
	if (m_Status.m_UserID > 0)
		return m_Status.m_UserID;

	return std::nullopt;
}

duration_t Player::GetConnectedTime() const
{
	auto result = GetWorld().GetCurrentTime() - GetConnectionTime();
	//assert(result >= -1s);
	result = std::max<duration_t>(result, 0s);
	return result;
}

const mh::expected<SteamAPI::PlayerSummary>& Player::GetPlayerSummary() const
{
	if (!m_PlayerSummary && m_PlayerSummary.error() == ErrorCode::LazyValueUninitialized)
	{
		m_PlayerSummary = std::errc::operation_in_progress;
		m_World->QueuePlayerSummaryUpdate(GetSteamID());
	}

	return m_PlayerSummary;
}

const mh::expected<SteamAPI::PlayerBans>& Player::GetPlayerBans() const
{
	if (!m_PlayerSteamBans && m_PlayerSteamBans.error() == ErrorCode::LazyValueUninitialized)
	{
		m_PlayerSteamBans = std::errc::operation_in_progress;
		m_World->QueuePlayerBansUpdate(GetSteamID());
	}

	return m_PlayerSteamBans;
}

const mh::expected<SteamHistoryAPI::PlayerSourceBanState>& Player::GetPlayerSourceBanState() const
{
	if (!m_PlayerSourceBanState && m_PlayerSourceBanState.error() == ErrorCode::LazyValueUninitialized)
	{
		m_PlayerSourceBanState = std::errc::operation_in_progress;
		m_PlayerSourceBans = std::errc::operation_in_progress;
		m_World->QueuePlayerSourceBansUpdate(GetSteamID());
	}

	return m_PlayerSourceBanState;
}

template<typename T, typename TFunc>
const mh::expected<T>& Player::GetOrFetchDataAsync(mh::expected<T>& var, TFunc&& updateFunc,
	std::initializer_list<std::error_condition> silentErrors, const mh::source_location& location) const
{
	m_Sentinel.check(location);

	if (var == ErrorCode::LazyValueUninitialized ||
		var == ErrorCode::InternetConnectivityDisabled)
	{
		auto client = m_World->GetSettings().GetHTTPClient();

		if (!client)
		{
			var = ErrorCode::InternetConnectivityDisabled;
		}
		else
		{
			var = std::errc::operation_in_progress;

			auto sharedThis = shared_from_this();

			[](std::shared_ptr<const Player> sharedThis, std::shared_ptr<const IHTTPClient> client,
				mh::expected<T>& var, std::vector<std::error_condition> silentErrors, TFunc updateFunc,
				mh::source_location location) -> mh::task<>
			{
				try
				{
					mh::expected<T> result;
					try
					{
						result = co_await updateFunc(sharedThis, client);
					}
					catch (const std::system_error& e)
					{
						result = e.code().default_error_condition();

						if (!mh::contains(silentErrors, result.error()))
							DebugLogException(location, e);
					}
					catch (...)
					{
						LogException(location);
						result = ErrorCode::UnknownError;
					}

					co_await GetDispatcher().co_dispatch();  // switch to main thread

					var = std::move(result);
				}
				catch (...)
				{
					LogException(location);
				}

			}(sharedThis, client, var, silentErrors, std::move(updateFunc), location);
		}
	}

	return var;
}

const mh::expected<LogsTFAPI::PlayerLogsInfo>& Player::GetLogsInfo() const
{
	return GetOrFetchDataAsync(m_LogsInfo,
		[&](std::shared_ptr<const Player> pThis, auto client) -> mh::task<LogsTFAPI::PlayerLogsInfo>
		{
			DB::ITempDB& cacheDB = TF2BDApplication::GetApplication().GetTempDB();

			DB::LogsTFCacheInfo cacheInfo{};
			cacheInfo.m_ID = pThis->GetSteamID();

			co_await cacheDB.GetOrUpdateAsync(cacheInfo, [&client](DB::LogsTFCacheInfo& info) -> mh::task<>
				{
					info = co_await LogsTFAPI::GetPlayerLogsInfoAsync(client, info.m_ID);
				});

			co_return cacheInfo;
		});
}

const mh::expected<SteamAPI::PlayerFriends>& Player::GetFriendsInfo() const
{
	// TODO: use tempdb?
	return GetOrFetchDataAsync(m_FriendsInfo,
		[&](std::shared_ptr<const Player> pThis, auto client) -> mh::task< mh::expected<SteamAPI::PlayerFriends>>
		{
			const auto& settings = pThis->GetWorld().GetSettings();
			if (!settings.IsSteamAPIAvailable())
				co_return SteamAPI::ErrorCode::SteamAPIDisabled;

			SteamAPI::PlayerFriends data;

			data.m_Friends = co_await SteamAPI::GetFriendList(settings, pThis->GetSteamID(), *client);

			co_return data;
		});
}

const mh::expected<SteamAPI::PlayerInventoryInfo>& Player::GetInventoryInfo() const
{
	return GetOrFetchDataAsync(m_InventoryInfo,
		[&](std::shared_ptr<const Player> pThis, auto client) -> mh::task<mh::expected<SteamAPI::PlayerInventoryInfo>>
		{
			DB::ITempDB& cacheDB = TF2BDApplication::GetApplication().GetTempDB();

			DB::AccountInventorySizeInfo cacheInfo{};
			cacheInfo.m_SteamID = pThis->GetSteamID();
			cacheInfo.m_Items = cacheInfo.m_Slots = (uint32_t)-1; // sanity check

			const auto& settings = pThis->GetWorld().GetSettings();
			if (!settings.IsSteamAPIAvailable())
				co_return SteamAPI::ErrorCode::SteamAPIDisabled;

			co_await cacheDB.GetOrUpdateAsync(cacheInfo, [&settings, &client](DB::AccountInventorySizeInfo& info) -> mh::task<>
				{
					info = co_await SteamAPI::GetTF2InventoryInfoAsync(settings, info.GetSteamID(), *client);
				});

			co_return cacheInfo;
		});
}

mh::expected<duration_t> Player::GetTF2Playtime() const
{
	using ErrorCode = SteamAPI::ErrorCode;

	return GetOrFetchDataAsync(m_TF2Playtime,
		[&](std::shared_ptr<const Player> pThis, std::shared_ptr<const IHTTPClient> client) -> mh::task<mh::expected<duration_t>>
		{
			const auto& settings = pThis->GetWorld().GetSettings();
			if (!settings.IsSteamAPIAvailable())
				co_return ErrorCode::SteamAPIDisabled;

			co_return co_await SteamAPI::GetTF2PlaytimeAsync(settings, GetSteamID(), *client);
		}, { ErrorCode::InfoPrivate, ErrorCode::GameNotOwned });
}

bool Player::IsFriend() const
{
	return m_World->GetFriends().contains(GetSteamID());
}

duration_t Player::GetActiveTime() const
{
	if (m_Status.m_State != PlayerStatusState::Active)
		return 0s;

	return m_LastStatusUpdateTime - m_LastStatusActiveBegin;
}

std::optional<time_point_t> Player::GetEstimatedAccountCreationTime() const
{
	if (auto& summary = GetPlayerSummary())
	{
		if (summary->GetAccountAge())
			return summary->m_CreationTime;
	}

	return m_World->GetAccountAges().EstimateAccountCreationTime(GetSteamID());
}

void Player::SetStatus(PlayerStatus status, time_point_t timestamp)
{
	if (m_Status.m_State != PlayerStatusState::Active && status.m_State == PlayerStatusState::Active)
		m_LastStatusActiveBegin = timestamp;

	m_Status = std::move(status);
	m_LastStatusUpdateTime = m_LastPingUpdateTime = timestamp;
}

void Player::SetPing(uint16_t ping, time_point_t timestamp)
{
	m_Status.m_Ping = ping;
	m_LastPingUpdateTime = timestamp;
}

const std::any* Player::FindDataStorage(const std::type_index& type) const
{
	if (auto found = m_UserData.find(type); found != m_UserData.end())
		return &found->second;

	return nullptr;
}

std::any& Player::GetOrCreateDataStorage(const std::type_index& type)
{
	return m_UserData[type];
}
