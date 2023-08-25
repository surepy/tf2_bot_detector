#pragma once
#include "Clock.h"
#include "SteamID.h"
#include "WorldState.h"
#include "BatchedAction.h"
#include "GenericErrors.h"

#include "Networking/SteamAPI.h"
#include "Networking/LogsTFAPI.h"
#include "Networking/SteamHistoryAPI.h"

#include "GameData/IPlayer.h"

#include "ConsoleLog/ConsoleLogParser.h"
#include "ConsoleLog/ConsoleLineListener.h"

#include <mh/concurrency/thread_sentinel.hpp>

namespace tf2_bot_detector
{
	class Player final : public IPlayer
	{
	public:
		Player(tf2_bot_detector::WorldState& world, SteamID id);

		tf2_bot_detector::WorldState& GetWorld() override { return static_cast<tf2_bot_detector::WorldState&>(IPlayer::GetWorld()); }
		const tf2_bot_detector::WorldState& GetWorld() const override;
		const LobbyMember* GetLobbyMember() const override;
		std::string GetNameUnsafe() const override { return m_Status.m_Name; }
		tf2_bot_detector::SteamID GetSteamID() const override { return m_Status.m_SteamID; }
		PlayerStatusState GetConnectionState() const override { return m_Status.m_State; }
		std::optional<UserID_t> GetUserID() const override;
		TFTeam GetTeam() const override { return m_Team; }
		time_point_t GetConnectionTime() const override { return m_Status.m_ConnectionTime; }
		duration_t GetConnectedTime() const override;
		const PlayerScores& GetScores() const override { return m_Scores; }
		uint16_t GetPing() const override { return m_Status.m_Ping; }
		time_point_t GetLastStatusUpdateTime() const override { return m_LastStatusUpdateTime; }
		const mh::expected<SteamAPI::PlayerSummary>& GetPlayerSummary() const override;
		const mh::expected<SteamAPI::PlayerBans>& GetPlayerBans() const override;
		const mh::expected<SteamHistoryAPI::PlayerSourceBanState>& GetPlayerSourceBanState() const override;
		//const mh::expected<SteamHistoryAPI::PlayerSourceBans>& GetPlayerSourceBans() const override;

		mh::expected<duration_t> GetTF2Playtime() const override;
		bool IsFriend() const override;
		duration_t GetActiveTime() const override;

		std::optional<time_point_t> GetEstimatedAccountCreationTime() const override;

		const mh::expected<LogsTFAPI::PlayerLogsInfo>& GetLogsInfo() const override;
		const mh::expected<SteamAPI::PlayerFriends>& GetFriendsInfo() const override;
		const mh::expected<SteamAPI::PlayerInventoryInfo>& GetInventoryInfo() const override;

		PlayerScores m_Scores{};
		TFTeam m_Team{};

		uint8_t m_ClientIndex{};
		mutable mh::expected<SteamAPI::PlayerSummary> m_PlayerSummary = ErrorCode::LazyValueUninitialized;
		mutable mh::expected<SteamAPI::PlayerBans> m_PlayerSteamBans = ErrorCode::LazyValueUninitialized;

		mutable mh::expected<SteamHistoryAPI::PlayerSourceBans> m_PlayerSourceBans = ErrorCode::LazyValueUninitialized;
		mutable mh::expected<SteamHistoryAPI::PlayerSourceBanState> m_PlayerSourceBanState = ErrorCode::LazyValueUninitialized;

		void SetStatus(PlayerStatus status, time_point_t timestamp);
		const PlayerStatus& GetStatus() const { return m_Status; }

		void SetPing(uint16_t ping, time_point_t timestamp);

	protected:
		std::map<std::type_index, std::any> m_UserData;
		const std::any* FindDataStorage(const std::type_index& type) const override;
		std::any& GetOrCreateDataStorage(const std::type_index& type) override;

		std::shared_ptr<Player> shared_from_this() { return std::static_pointer_cast<Player>(IPlayer::shared_from_this()); }
		std::shared_ptr<const Player> shared_from_this() const { return std::static_pointer_cast<const Player>(IPlayer::shared_from_this()); }

	private:
		mh::thread_sentinel m_Sentinel;

		template<typename T, typename TFunc>
		const mh::expected<T>& GetOrFetchDataAsync(mh::expected<T>& variable, TFunc&& updateFunc,
			std::initializer_list<std::error_condition> silentErrors = {}, MH_SOURCE_LOCATION_AUTO(location)) const;

		WorldState* m_World = nullptr;
		PlayerStatus m_Status{};

		time_point_t m_LastStatusActiveBegin{};

		time_point_t m_LastStatusUpdateTime{};
		time_point_t m_LastPingUpdateTime{};

		mutable mh::expected<duration_t> m_TF2Playtime = ErrorCode::LazyValueUninitialized;
		mutable mh::expected<LogsTFAPI::PlayerLogsInfo> m_LogsInfo = ErrorCode::LazyValueUninitialized;
		mutable mh::expected<SteamAPI::PlayerFriends> m_FriendsInfo = ErrorCode::LazyValueUninitialized;
		mutable mh::expected<SteamAPI::PlayerInventoryInfo> m_InventoryInfo = ErrorCode::LazyValueUninitialized;
	};
}
