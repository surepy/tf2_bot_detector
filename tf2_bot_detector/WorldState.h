#pragma once

#include "Clock.h"
#include "SteamID.h"
#include "GameData/TFConstants.h"

#include <mh/coroutine/task.hpp>
#include <mh/coroutine/generator.hpp>

#include <optional>

#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"

#include "ConsoleLog/ConsoleLines/ChatConsoleLine.h"
#include "ConsoleLog/ConsoleLines/LobbyHeaderLine.h"
#include "ConsoleLog/ConsoleLines/LobbyMemberLine.h"
#include "ConsoleLog/ConsoleLines/LobbyStatusFailedLine.h"
#include "ConsoleLog/ConsoleLines/ServerStatusPlayerLine.h"
#include "ConsoleLog/ConsoleLines/KillNotificationLine.h"
#include "ConsoleLog/ConsoleLines/ConfigExecLine.h"
#include "ConsoleLog/ConsoleLines/SVCUserMessageLine.h"
#include "ConsoleLog/ConsoleLines/ServerStatusShortPlayerLine.h"
#include "ConsoleLog/ConsoleLines/LobbyChangedLine.h"
#include "ConsoleLog/ConsoleLines/ServerDroppedPlayerLine.h"
#include "ConsoleLog/ConsoleLines/PingLine.h"

#include "Config/AccountAges.h"

#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"
#include "BatchedAction.h"
#include <mh/algorithm/algorithm.hpp>
#include <mh/concurrency/dispatcher.hpp>
#include <mh/concurrency/main_thread.hpp>
#include <mh/concurrency/thread_pool.hpp>
#include <mh/concurrency/thread_sentinel.hpp>
#include <mh/future.hpp>
#include <mh/coroutine/future.hpp>

#undef GetCurrentTime

namespace tf2_bot_detector
{
	class ChatConsoleLine;
	class ConfigExecLine;
	class ConsoleLogParser;
	class IAccountAges;
	class IConsoleLineListener;
	class IPlayer;
	class Player;
	class IWorldEventListener;
	enum class LobbyMemberTeam : uint8_t;
	class Settings;
	enum class TFClassType;

	enum class TeamShareResult
	{
		SameTeams,
		OppositeTeams,
		Neither,
	};

	class IWorldState;

	class IWorldStateConLog
	{
	public:
		virtual ~IWorldStateConLog() = default;

	private:
		friend class ConsoleLogParser;

		virtual IConsoleLineListener& GetConsoleLineListenerBroadcaster() = 0;

		virtual void UpdateTimestamp(const ConsoleLogParser& parser) = 0;
	};

	class IWorldState : public IWorldStateConLog, public std::enable_shared_from_this<IWorldState>
	{
	public:
		virtual ~IWorldState() = default;

		static std::shared_ptr<IWorldState> Create(const Settings& settings);

		virtual void Update() = 0;
		virtual void ResetScoreboard() = 0;

		virtual time_point_t GetCurrentTime() const = 0;
		virtual time_point_t GetLastStatusUpdateTime() const = 0;

		virtual void AddWorldEventListener(IWorldEventListener* listener) = 0;
		virtual void RemoveWorldEventListener(IWorldEventListener* listener) = 0;
		virtual void AddConsoleLineListener(IConsoleLineListener* listener) = 0;
		virtual void RemoveConsoleLineListener(IConsoleLineListener* listener) = 0;

		virtual void AddConsoleOutputChunk(const std::string_view& chunk) = 0;
		virtual mh::task<> AddConsoleOutputLine(std::string line) = 0;

		virtual std::optional<SteamID> FindSteamIDForName(const std::string_view& playerName) const = 0;
		virtual std::optional<LobbyMemberTeam> FindLobbyMemberTeam(const SteamID& id) const = 0;
		virtual std::optional<UserID_t> FindUserID(const SteamID& id) const = 0;

		virtual TeamShareResult GetTeamShareResult(const SteamID& id) const = 0;
		virtual TeamShareResult GetTeamShareResult(const SteamID& id0, const SteamID& id1) const = 0;
		virtual TeamShareResult GetTeamShareResult(const std::optional<LobbyMemberTeam>& team0, const SteamID& id1) const = 0;
		static TeamShareResult GetTeamShareResult(const std::optional<LobbyMemberTeam>& team0, const std::optional<LobbyMemberTeam>& team1);

		virtual const IPlayer* FindPlayer(const SteamID& id) const = 0;
		IPlayer* FindPlayer(const SteamID& id) { return const_cast<IPlayer*>(std::as_const(*this).FindPlayer(id)); }
		virtual const IPlayer* LocalPlayer() const = 0;

		virtual size_t GetApproxLobbyMemberCount() const = 0;
		virtual mh::generator<const IPlayer&> GetLobbyMembers() const = 0;
		mh::generator<IPlayer&> GetLobbyMembers();
		virtual mh::generator<const IPlayer&> GetPlayers() const = 0;
		mh::generator<IPlayer&> GetPlayers();

		// Have we joined a team and picked a class?
		virtual bool IsLocalPlayerInitialized() const = 0;
		virtual bool IsVoteInProgress() const = 0;

		virtual const std::string& GetServerHostName() const = 0;
		virtual const std::string& GetMapName() const = 0;

		virtual const IAccountAges& GetAccountAges() const = 0;
	};

	inline mh::generator<IPlayer&> IWorldState::GetLobbyMembers()
	{
		for (const IPlayer& p : std::as_const(*this).GetLobbyMembers())
			co_yield const_cast<IPlayer&>(p);
	}
	inline mh::generator<IPlayer&> IWorldState::GetPlayers()
	{
		for (const IPlayer& p : std::as_const(*this).GetPlayers())
			co_yield const_cast<IPlayer&>(p);
	}

	class WorldState final : public IWorldState, BaseConsoleLineListener
	{
	public:
		WorldState(const Settings& settings);
		~WorldState();

		std::shared_ptr<WorldState> shared_from_this() { return std::static_pointer_cast<WorldState>(IWorldState::shared_from_this()); }
		std::shared_ptr<const WorldState> shared_from_this() const { return std::static_pointer_cast<const WorldState>(IWorldState::shared_from_this()); }

		time_point_t GetCurrentTime() const override { return m_CurrentTimestamp.GetSnapshot(); }
		size_t GetApproxLobbyMemberCount() const override;

		void Update() override;
		void UpdateTimestamp(const ConsoleLogParser& parser);
		void ResetScoreboard() override;

		void AddWorldEventListener(IWorldEventListener* listener) override;
		void RemoveWorldEventListener(IWorldEventListener* listener) override;
		void AddConsoleLineListener(IConsoleLineListener* listener) override;
		void RemoveConsoleLineListener(IConsoleLineListener* listener) override;

		void AddConsoleOutputChunk(const std::string_view& chunk) override;
		mh::task<> AddConsoleOutputLine(std::string line) override;

		std::optional<SteamID> FindSteamIDForName(const std::string_view& playerName) const override;
		std::optional<LobbyMemberTeam> FindLobbyMemberTeam(const SteamID& id) const override;
		std::optional<UserID_t> FindUserID(const SteamID& id) const override;

		TeamShareResult GetTeamShareResult(const SteamID& id) const override;
		TeamShareResult GetTeamShareResult(const SteamID& id0, const SteamID& id1) const override;
		TeamShareResult GetTeamShareResult(const std::optional<LobbyMemberTeam>& team0, const SteamID& id1) const override;
		static TeamShareResult GetTeamShareResult(
			const std::optional<LobbyMemberTeam>& team0, const std::optional<LobbyMemberTeam>& team1);

		using IWorldState::FindPlayer;
		const IPlayer* FindPlayer(const SteamID& id) const override;
		const IPlayer* LocalPlayer() const override;

		mh::generator<const IPlayer&> GetLobbyMembers() const;
		mh::generator<const IPlayer&> GetPlayers() const;
		std::vector<const IPlayer*> GetRecentPlayers(size_t recentPlayerCount = 32) const;
		std::vector<IPlayer*> GetRecentPlayers(size_t recentPlayerCount = 32);

		time_point_t GetLastStatusUpdateTime() const { return m_LastStatusUpdateTime; }

		// Have we joined a team and picked a class?
		bool IsLocalPlayerInitialized() const override { return m_IsLocalPlayerInitialized; }
		bool IsVoteInProgress() const override { return m_IsVoteInProgress; }

		void QueuePlayerSummaryUpdate(const SteamID& id);
		void QueuePlayerBansUpdate(const SteamID& id);
		void QueuePlayerSourceBansUpdate(const SteamID& id);

		const Settings& GetSettings() const { return m_Settings; }
		const std::vector<LobbyMember>& GetCurrentLobbyMembers() const { return m_CurrentLobbyMembers; }
		const std::vector<LobbyMember>& GetPendingLobbyMembers() const { return m_PendingLobbyMembers; }
		const std::unordered_set<SteamID>& GetFriends() const { return m_Friends; }

		IAccountAges& GetAccountAges() { return *m_AccountAges; }
		const IAccountAges& GetAccountAges() const override { return *m_AccountAges; }

		const std::string& GetServerHostName() const override { return m_ServerHostName; }
		const std::string& GetMapName() const override { return m_MapName; }

	protected:
		virtual IConsoleLineListener& GetConsoleLineListenerBroadcaster() { return m_ConsoleLineListenerBroadcaster; }

	private:
		const Settings& m_Settings;

		CompensatedTS m_CurrentTimestamp;

		void OnConsoleLineParsed(IWorldState& world, IConsoleLine& parsed) override;
		void OnConfigExecLineParsed(const ConfigExecLine& execLine);

		void UpdateFriends();
		mh::task<std::unordered_set<SteamID>> m_FriendsFuture;
		std::unordered_set<SteamID> m_Friends;
		time_point_t m_LastFriendsUpdate{};

		Player& FindOrCreatePlayer(const SteamID& id);

		struct PlayerSummaryUpdateAction final :
			BatchedAction<WorldState*, SteamID, std::vector<SteamAPI::PlayerSummary>>
		{
			using BatchedAction::BatchedAction;
		protected:
			response_future_type SendRequest(WorldState*& state, queue_collection_type& collection) override;
			void OnDataReady(WorldState*& state, const response_type& response,
				queue_collection_type& collection) override;
		} m_PlayerSummaryUpdates;

		struct PlayerBansUpdateAction final :
			BatchedAction<WorldState*, SteamID, std::vector<SteamAPI::PlayerBans>>
		{
			using BatchedAction::BatchedAction;
		protected:
			response_future_type SendRequest(state_type& state, queue_collection_type& collection) override;
			void OnDataReady(state_type& state, const response_type& response,
				queue_collection_type& collection) override;
		} m_PlayerBansUpdates;

		struct PlayerSourceBansUpdateAction final :
			BatchedAction<WorldState*, SteamID, SteamHistoryAPI::PlayerSourceBansResponse>
		{
			using BatchedAction::BatchedAction;
		protected:
			response_future_type SendRequest(state_type& state, queue_collection_type& collection) override;
			void OnDataReady(state_type& state, const response_type& response,
				queue_collection_type& collection) override;
		} m_PlayerSourceBansUpdates;

		std::string m_ServerHostName;
		std::string m_MapName;

		std::vector<LobbyMember> m_CurrentLobbyMembers;
		std::vector<LobbyMember> m_PendingLobbyMembers;
		std::unordered_map<SteamID, std::shared_ptr<Player>> m_CurrentPlayerData;
		bool m_IsLocalPlayerInitialized = false;
		bool m_IsVoteInProgress = false;

		std::shared_ptr<tf2_bot_detector::IAccountAges> m_AccountAges = tf2_bot_detector::IAccountAges::Create();

		time_point_t m_LastStatusUpdateTime{};

		std::unordered_set<IConsoleLineListener*> m_ConsoleLineListeners;
		std::unordered_set<IWorldEventListener*> m_EventListeners;

		mh::thread_pool m_ConsoleLineParsingPool{ 1 };
		std::vector<mh::shared_future<std::shared_ptr<IConsoleLine>>> m_ConsoleLineParsingTasks;

		struct ConsoleLineListenerBroadcaster final : IConsoleLineListener
		{
			ConsoleLineListenerBroadcaster(WorldState& world) : m_World(world) {}

			void OnConsoleLineParsed(IWorldState& world, IConsoleLine& line) override
			{
				for (IConsoleLineListener* l : m_World.m_ConsoleLineListeners)
					l->OnConsoleLineParsed(world, line);
			}
			void OnConsoleLineUnparsed(IWorldState& world, const std::string_view& text) override
			{
				for (IConsoleLineListener* l : m_World.m_ConsoleLineListeners)
					l->OnConsoleLineUnparsed(world, text);
			}
			void OnConsoleLogChunkParsed(IWorldState& world, bool consoleLinesParsed) override
			{
				for (IConsoleLineListener* l : m_World.m_ConsoleLineListeners)
					l->OnConsoleLogChunkParsed(world, consoleLinesParsed);
			}

			WorldState& m_World;

		} m_ConsoleLineListenerBroadcaster;
		template<typename TRet, typename... TArgs, typename... TArgs2>
		inline void InvokeEventListener(TRet(IWorldEventListener::* func)(TArgs... args), TArgs2&&... args)
		{
			for (IWorldEventListener* listener : m_EventListeners)
				(listener->*func)(std::forward<TArgs2>(args)...);
		}
	};
}
