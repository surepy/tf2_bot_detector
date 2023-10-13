#include "WorldState.h"
#include "Actions/Actions.h"
#include "Config/Settings.h"
#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"
#include "GameData/TFClassType.h"
#include "GameData/UserMessageType.h"
#include "Networking/HTTPHelpers.h"
#include "Networking/SteamAPI.h"
#include "Networking/SteamHistoryAPI.h"
#include "Networking/LogsTFAPI.h"
#include "Util/RegexUtils.h"
#include "Util/TextUtils.h"
#include "BatchedAction.h"
#include "GenericErrors.h"
#include "GameData/IPlayer.h"
#include "GameData/Player.h"
#include "Log.h"
#include "WorldEventListener.h"
#include "Config/AccountAges.h"
#include "GlobalDispatcher.h"
#include "Application.h"
#include "DB/TempDB.h"

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

#include <mh/algorithm/algorithm.hpp>
#include <mh/concurrency/dispatcher.hpp>
#include <mh/concurrency/main_thread.hpp>
#include <mh/concurrency/thread_pool.hpp>
#include <mh/concurrency/thread_sentinel.hpp>
#include <mh/future.hpp>
#include <mh/coroutine/future.hpp>

#undef GetCurrentTime
#undef max
#undef min

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace tf2_bot_detector;

std::shared_ptr<IWorldState> IWorldState::Create(const Settings& settings)
{
	return std::make_shared<WorldState>(settings);
}

WorldState::WorldState(const Settings& settings) :
	m_Settings(settings),
	m_PlayerSummaryUpdates(this),
	m_PlayerBansUpdates(this),
	m_PlayerSourceBansUpdates(this),
	m_ConsoleLineListenerBroadcaster(*this)
{
	AddConsoleLineListener(this);
}

WorldState::~WorldState()
{
	RemoveConsoleLineListener(this);
}

void WorldState::Update()
{
	m_PlayerSummaryUpdates.Update();
	m_PlayerBansUpdates.Update();
	m_PlayerSourceBansUpdates.Update();

	UpdateFriends();
}

void WorldState::UpdateFriends()
{
	// only warn about our friends list failing to update warning once.
	static bool s_HasWarnedFriendsUpdateFailure = false;

	if (auto client = GetSettings().GetHTTPClient();
		client && GetSettings().IsSteamAPIAvailable() && (tfbd_clock_t::now() - 5min) > m_LastFriendsUpdate)
	{
		m_LastFriendsUpdate = tfbd_clock_t::now();
		m_FriendsFuture = SteamAPI::GetFriendList(GetSettings(), GetSettings().GetLocalSteamID(), *client);
		// we can warn about our friends list failing again because this is a new api request.
		s_HasWarnedFriendsUpdateFailure = false;
	}

	if (m_FriendsFuture.is_ready())
	{
		const auto GenericException = [this](const mh::source_location& loc)
		{
			LogException(loc, "Failed to update our friends list");
			m_FriendsFuture = {};
		};

		try
		{
			m_Friends = m_FriendsFuture.get();
		}
		catch (const http_error& e)
		{
			// TODO: investigate if there's a way to get friends list on private profiles
			// easiest: call functions directly from tf2's steam_api.dll (a ton of resources)
			if (e.code() == HTTPResponseCode::Unauthorized)
			{
				constexpr const char WARNING_MSG[] = "Failed to access our friends list (our friends list is private/friends only). The tool will not be able to show who is friends with you.";

				if (!s_HasWarnedFriendsUpdateFailure)
				{
					LogWarning(WARNING_MSG);
					s_HasWarnedFriendsUpdateFailure = true;
				}
			}
			else
			{
				GenericException(MH_SOURCE_LOCATION_CURRENT());
			}
		}
		catch (...)
		{
			GenericException(MH_SOURCE_LOCATION_CURRENT());
		}
	}
}

void WorldState::AddConsoleLineListener(IConsoleLineListener* listener)
{
	m_ConsoleLineListeners.insert(listener);
}

void WorldState::RemoveConsoleLineListener(IConsoleLineListener* listener)
{
	m_ConsoleLineListeners.erase(listener);
}

void WorldState::AddConsoleOutputChunk(const std::string_view& chunk)
{
	size_t last = 0;
	for (auto i = chunk.find('\n', 0); i != chunk.npos; i = chunk.find('\n', last))
	{
		auto line = chunk.substr(last, i - last);
		AddConsoleOutputLine(std::string(line));
		last = i + 1;
	}
}

mh::task<> WorldState::AddConsoleOutputLine(std::string line)
{
	auto worldState = shared_from_this();

	// Switch to thread "pool" thread (there is only 1 thread in this particular pool)
	co_await m_ConsoleLineParsingPool.co_add_task();

	auto parsed = IConsoleLine::ParseConsoleLine(line, GetCurrentTime(), *this);

	// switch to main thread
	co_await GetDispatcher().co_dispatch();

	if (parsed)
	{
		for (auto listener : m_ConsoleLineListeners)
			listener->OnConsoleLineParsed(*worldState, *parsed);
	}
	else
	{
		for (auto listener : m_ConsoleLineListeners)
			listener->OnConsoleLineUnparsed(*worldState, std::move(line));
	}
}

void WorldState::UpdateTimestamp(const ConsoleLogParser& parser)
{
	m_CurrentTimestamp = parser.GetCurrentTimestamp();
}

void WorldState::AddWorldEventListener(IWorldEventListener* listener)
{
	m_EventListeners.insert(listener);
}

void WorldState::RemoveWorldEventListener(IWorldEventListener* listener)
{
	m_EventListeners.erase(listener);
}

std::optional<SteamID> WorldState::FindSteamIDForName(const std::string_view& playerName) const
{
	std::optional<SteamID> retVal;
	time_point_t lastUpdated{};

	for (const auto& data : m_CurrentPlayerData)
	{
		if (data.second->GetStatus().m_Name == playerName && data.second->GetLastStatusUpdateTime() > lastUpdated)
		{
			retVal = data.second->GetSteamID();
			lastUpdated = data.second->GetLastStatusUpdateTime();
		}
	}

	return retVal;
}

std::optional<LobbyMemberTeam> WorldState::FindLobbyMemberTeam(const SteamID& id) const
{
	for (const auto& member : m_CurrentLobbyMembers)
	{
		if (member.m_SteamID == id)
			return member.m_Team;
	}

	for (const auto& member : m_PendingLobbyMembers)
	{
		if (member.m_SteamID == id)
			return member.m_Team;
	}

	return std::nullopt;
}

std::optional<UserID_t> WorldState::FindUserID(const SteamID& id) const
{
	for (const auto& player : m_CurrentPlayerData)
	{
		if (player.second->GetSteamID() == id)
			return player.second->GetUserID();
	}

	return std::nullopt;
}

TeamShareResult WorldState::GetTeamShareResult(const SteamID& id) const
{
	return GetTeamShareResult(id, GetSettings().GetLocalSteamID());
}

auto WorldState::GetTeamShareResult(const std::optional<LobbyMemberTeam>& team0,
	const SteamID& id1) const -> TeamShareResult
{
	return GetTeamShareResult(team0, FindLobbyMemberTeam(id1));
}

auto WorldState::GetTeamShareResult(const std::optional<LobbyMemberTeam>& team0,
	const std::optional<LobbyMemberTeam>& team1) -> TeamShareResult
{
	if (!team0)
		return TeamShareResult::Neither;
	if (!team1)
		return TeamShareResult::Neither;

	if (*team0 == *team1)
		return TeamShareResult::SameTeams;
	else if (*team0 == OppositeTeam(*team1))
		return TeamShareResult::OppositeTeams;
	else
		throw std::runtime_error("Unexpected team value(s)");
}

const IPlayer* WorldState::FindPlayer(const SteamID& id) const
{
	if (auto found = m_CurrentPlayerData.find(id); found != m_CurrentPlayerData.end())
		return found->second.get();

	return nullptr;
}

const IPlayer* tf2_bot_detector::WorldState::LocalPlayer() const
{
	return FindPlayer(m_Settings.GetLocalSteamID());
}

size_t WorldState::GetApproxLobbyMemberCount() const
{
	return m_CurrentLobbyMembers.size() + m_PendingLobbyMembers.size();
}

mh::generator<const IPlayer&> WorldState::GetLobbyMembers() const
{
	const auto GetPlayer = [&](const LobbyMember& member) -> const IPlayer*
	{
		assert(member != LobbyMember{});
		assert(member.m_SteamID.IsValid());

		if (auto found = m_CurrentPlayerData.find(member.m_SteamID); found != m_CurrentPlayerData.end())
		{
			[[maybe_unused]] const LobbyMember* testMember = found->second->GetLobbyMember();
			//assert(*testMember == member);
			return found->second.get();
		}
		else
		{
			throw std::runtime_error("Missing player for lobby member!");
		}
	};

	for (const auto& member : m_CurrentLobbyMembers)
	{
		if (!member.IsValid())
			continue;

		if (auto found = GetPlayer(member))
			co_yield *found;
	}
	for (const auto& member : m_PendingLobbyMembers)
	{
		if (!member.IsValid())
			continue;

		if (std::any_of(m_CurrentLobbyMembers.begin(), m_CurrentLobbyMembers.end(),
			[&](const LobbyMember& member2) { return member.m_SteamID == member2.m_SteamID; }))
		{
			// Don't return two different instances with the same steamid.
			continue;
		}

		if (auto found = GetPlayer(member))
			co_yield *found;
	}
}

mh::generator<const IPlayer&> WorldState::GetPlayers() const
{
	for (const auto& pair : m_CurrentPlayerData)
		co_yield *pair.second;
}

void WorldState::QueuePlayerSummaryUpdate(const SteamID& id)
{
	return m_PlayerSummaryUpdates.Queue(id);
}

void WorldState::QueuePlayerBansUpdate(const SteamID& id)
{
	return m_PlayerBansUpdates.Queue(id);
}

void WorldState::QueuePlayerSourceBansUpdate(const SteamID& id)
{
	return m_PlayerSourceBansUpdates.Queue(id);
}

template<typename TMap>
static auto GetRecentPlayersImpl(TMap&& map, size_t recentPlayerCount)
{
	using value_type = std::conditional_t<std::is_const_v<std::remove_reference_t<TMap>>, const IPlayer*, IPlayer*>;
	std::vector<value_type> retVal;

	for (auto& [sid, player] : map)
		retVal.push_back(player.get());

	std::sort(retVal.begin(), retVal.end(),
		[](const IPlayer* a, const IPlayer* b)
		{
			return b->GetLastStatusUpdateTime() < a->GetLastStatusUpdateTime();
		});

	if (retVal.size() > recentPlayerCount)
		retVal.resize(recentPlayerCount);

	return retVal;
}

std::vector<const IPlayer*> WorldState::GetRecentPlayers(size_t recentPlayerCount) const
{
	return GetRecentPlayersImpl(m_CurrentPlayerData, recentPlayerCount);
}

std::vector<IPlayer*> WorldState::GetRecentPlayers(size_t recentPlayerCount)
{
	return GetRecentPlayersImpl(m_CurrentPlayerData, recentPlayerCount);
}

void WorldState::OnConfigExecLineParsed(const ConfigExecLine& execLine)
{
	const std::string_view& cfgName = execLine.GetConfigFileName();
	if (cfgName.ends_with("scout.cfg"sv) ||
		cfgName.ends_with("sniper.cfg"sv) ||
		cfgName.ends_with("soldier.cfg"sv) ||
		cfgName.ends_with("demoman.cfg"sv) ||
		cfgName.ends_with("medic.cfg"sv) ||
		cfgName.ends_with("heavyweapons.cfg"sv) ||
		cfgName.ends_with("pyro.cfg"sv) ||
		cfgName.ends_with("spy.cfg"sv) ||
		cfgName.ends_with("engineer.cfg"sv))
	{
		DebugLog("Spawned as {}", cfgName.substr(0, cfgName.size() - 3));

		TFClassType cl = TFClassType::Undefined;
		if (cfgName.starts_with("scout"))
			cl = TFClassType::Scout;
		else if (cfgName.starts_with("sniper"))
			cl = TFClassType::Sniper;
		else if (cfgName.starts_with("soldier"))
			cl = TFClassType::Soldier;
		else if (cfgName.starts_with("demoman"))
			cl = TFClassType::Demoman;
		else if (cfgName.starts_with("medic"))
			cl = TFClassType::Medic;
		else if (cfgName.starts_with("heavyweapons"))
			cl = TFClassType::Heavy;
		else if (cfgName.starts_with("pyro"))
			cl = TFClassType::Pyro;
		else if (cfgName.starts_with("spy"))
			cl = TFClassType::Spy;
		else if (cfgName.starts_with("engineer"))
			cl = TFClassType::Engie;

		InvokeEventListener(&IWorldEventListener::OnLocalPlayerSpawned, *this, cl);

		if (!m_IsLocalPlayerInitialized)
		{
			m_IsLocalPlayerInitialized = true;
			InvokeEventListener(&IWorldEventListener::OnLocalPlayerInitialized, *this, m_IsLocalPlayerInitialized);
		}
	}
}

void WorldState::OnConsoleLineParsed(IWorldState& world, IConsoleLine& parsed)
{
	assert(&world == this);

	const auto ClearLobbyState = [&]
	{
		m_CurrentLobbyMembers.clear();
		m_PendingLobbyMembers.clear();
		m_CurrentPlayerData.clear();
	};

	switch (parsed.GetType())
	{
	case ConsoleLineType::LobbyHeader:
	{
		auto& headerLine = static_cast<const LobbyHeaderLine&>(parsed);
		m_CurrentLobbyMembers.resize(headerLine.GetMemberCount());
		m_PendingLobbyMembers.resize(headerLine.GetPendingCount());
		break;
	}
	case ConsoleLineType::LobbyStatusFailed:
	{
		if (!m_CurrentLobbyMembers.empty() || !m_PendingLobbyMembers.empty())
		{
			ClearLobbyState();
		}
		break;
	}
	case ConsoleLineType::LobbyChanged:
	{
		auto& lobbyChangedLine = static_cast<const LobbyChangedLine&>(parsed);
		const LobbyChangeType changeType = lobbyChangedLine.GetChangeType();

		if (changeType == LobbyChangeType::Created)
		{
			ClearLobbyState();

			if (m_Settings.m_SaveChatHistory) {
				// we should really probbaly not do this here like this, but what do i care
				ILogManager::GetInstance().LogChat("-------------------------------[LOBBY CHANGED]-------------------------------\n");
			}
		}

		if (changeType == LobbyChangeType::Created || changeType == LobbyChangeType::Updated)
		{
			// We can't trust the existing client indices
			for (auto& player : m_CurrentPlayerData)
				player.second->m_ClientIndex = 0;
		}
		break;
	}
	case ConsoleLineType::HostNewGame:
	case ConsoleLineType::Connecting:
	case ConsoleLineType::ClientReachedServerSpawn:
	{
		if (m_IsLocalPlayerInitialized)
		{
			m_IsLocalPlayerInitialized = false;
			InvokeEventListener(&IWorldEventListener::OnLocalPlayerInitialized, *this, m_IsLocalPlayerInitialized);
		}

		m_IsVoteInProgress = false;
		break;
	}
	case ConsoleLineType::Chat:
	{
		auto& chatLine = static_cast<const ChatConsoleLine&>(parsed);
		if (auto sid = FindSteamIDForName(chatLine.GetPlayerName()))
		{
			DebugLog("Chat message from {}: {}", *sid, std::quoted(chatLine.GetMessage()));
			if (auto player = FindPlayer(*sid))
			{
				InvokeEventListener(&IWorldEventListener::OnChatMsg, *this, *player, chatLine.GetMessage());
			}
			else
			{
				LogWarning("Dropped chat message with unknown IPlayer from {} ({})",
					std::quoted(chatLine.GetPlayerName()), std::quoted(chatLine.GetMessage()));
			}
		}
		else
		{
			LogWarning("Dropped chat message with unknown SteamID from {}: {}",
				std::quoted(chatLine.GetPlayerName()), std::quoted(chatLine.GetMessage()));
		}

		if (m_Settings.m_SaveChatHistory) {
			// we should probbaly not do this here like this, but what do i care
			// TODO: maybe make the function closer to how LogManager::Log() looks like
			std::ostringstream oss;

			// steamid
			oss << '<' << std::setw(17) << std::setfill('0') << chatLine.getSteamID().ID64 << "> ";

			// can't think of a good format for "YOU"
			//if (chatLine.IsSelf()) {}
			if (chatLine.IsDead()) {
				oss << "*DEAD* ";
			}

			if (chatLine.IsTeam()) {
				oss << "(TEAM) ";
			}

			oss << chatLine.GetPlayerName() << " : "
				<< chatLine.GetMessage()
				<< std::endl;

			ILogManager::GetInstance().LogChat(oss.str());
		}

		break;
	}
	case ConsoleLineType::ServerDroppedPlayer:
	{
		auto& dropLine = static_cast<const ServerDroppedPlayerLine&>(parsed);
		if (auto sid = FindSteamIDForName(dropLine.GetPlayerName()))
		{
			if (auto player = FindPlayer(*sid))
			{
				InvokeEventListener(&IWorldEventListener::OnPlayerDroppedFromServer,
					*this, *player, dropLine.GetReason());
			}
			else
			{
				LogWarning("Dropped \"player dropped\" message with unknown IPlayer from {} ({})",
					std::quoted(dropLine.GetPlayerName()), *sid);
			}
		}
		else
		{
			LogWarning("Dropped \"player dropped\" message with unknown SteamID from {}",
				std::quoted(dropLine.GetPlayerName()));
		}
		break;
	}
	case ConsoleLineType::ConfigExec:
	{
		OnConfigExecLineParsed(static_cast<const ConfigExecLine&>(parsed));
		break;
	}

	case ConsoleLineType::LobbyMember:
	{
		auto& memberLine = static_cast<const LobbyMemberLine&>(parsed);
		const auto& member = memberLine.GetLobbyMember();
		auto& vec = member.m_Pending ? m_PendingLobbyMembers : m_CurrentLobbyMembers;
		if (member.m_Index < vec.size())
			vec[member.m_Index] = member;

		const TFTeam tfTeam = member.m_Team == LobbyMemberTeam::Defenders ? TFTeam::Red : TFTeam::Blue;
		FindOrCreatePlayer(member.m_SteamID).m_Team = tfTeam;

		break;
	}
	case ConsoleLineType::Ping:
	{
		auto& pingLine = static_cast<const PingLine&>(parsed);
		if (auto found = FindSteamIDForName(pingLine.GetPlayerName()))
		{
			auto& playerData = FindOrCreatePlayer(*found);
			playerData.SetPing(pingLine.GetPing(), pingLine.GetTimestamp());
		}

		break;
	}
	case ConsoleLineType::PlayerStatus:
	{
		auto& statusLine = static_cast<const ServerStatusPlayerLine&>(parsed);
		auto newStatus = statusLine.GetPlayerStatus();
		auto& playerData = FindOrCreatePlayer(newStatus.m_SteamID);

		// Don't introduce stutter to our connection time view
		if (auto delta = (playerData.GetStatus().m_ConnectionTime - newStatus.m_ConnectionTime);
			delta < 2s && delta > -2s)
		{
			newStatus.m_ConnectionTime = playerData.GetStatus().m_ConnectionTime;
		}

		assert(playerData.GetStatus().m_SteamID == newStatus.m_SteamID);
		playerData.SetStatus(newStatus, statusLine.GetTimestamp());
		m_LastStatusUpdateTime = std::max(m_LastStatusUpdateTime, playerData.GetLastStatusUpdateTime());
		InvokeEventListener(&IWorldEventListener::OnPlayerStatusUpdate, *this, playerData);

		break;
	}
	case ConsoleLineType::PlayerStatusShort:
	{
		auto& statusLine = static_cast<const ServerStatusShortPlayerLine&>(parsed);
		const auto& status = statusLine.GetPlayerStatus();
		if (auto steamID = FindSteamIDForName(status.m_Name))
			FindOrCreatePlayer(*steamID).m_ClientIndex = status.m_ClientIndex;

		break;
	}
	case ConsoleLineType::KillNotification:
	{
		auto& killLine = static_cast<const KillNotificationLine&>(parsed);
		const auto localSteamID = GetSettings().GetLocalSteamID();
		const auto attackerSteamID = FindSteamIDForName(killLine.GetAttackerName());
		const auto victimSteamID = FindSteamIDForName(killLine.GetVictimName());

		// i don't like doing like this, but this is the quickest way to implement this.
		// i love tech debt.
		std::ostringstream killLogStream;

		if (attackerSteamID)
		{
			auto& attacker = FindOrCreatePlayer(*attackerSteamID);
			attacker.m_Scores.m_Kills++;

			if (victimSteamID == localSteamID)
				attacker.m_Scores.m_LocalKills++;

			killLogStream << "<" << std::setw(17) << std::setfill('0') << attacker.GetSteamID().ID64 << "> " << attacker.GetNameSafe();
		}

		killLogStream << " -> ";

		if (victimSteamID)
		{
			auto& victim = FindOrCreatePlayer(*victimSteamID);
			victim.m_Scores.m_Deaths++;

			if (attackerSteamID == localSteamID)
				victim.m_Scores.m_LocalDeaths++;


			killLogStream << "<" << victim.GetSteamID().ID64 << "> " << victim.GetNameSafe();
		}

		killLogStream << " // " << killLine.GetWeaponName() << (killLine.WasCrit() ? " (crit)" : "") << std::endl;

		if (m_Settings.m_KillLogsInChat) {
			// this looks messy af, i'm gonna add another setting value
			// ILogManager::GetInstance().LogChat(killLogStream.str());
		}

		break;
	}
	case ConsoleLineType::SVC_UserMessage:
	{
		auto& userMsg = static_cast<const SVCUserMessageLine&>(parsed);
		switch (userMsg.GetUserMessageType())
		{
		case UserMessageType::VoteStart:
			m_IsVoteInProgress = true;
			break;
		case UserMessageType::VoteFailed:
		case UserMessageType::VotePass:
			m_IsVoteInProgress = false;
			break;
		default:
			break;
		}

		break;
	}

#if 0
	case ConsoleLineType::NetDataTotal:
	{
		auto& netDataLine = static_cast<const NetDataTotalLine&>(parsed);
		auto ts = round_time_point(netDataLine.GetTimestamp(), 100ms);
		m_NetSamplesIn.m_Data[ts].AddSample(netDataLine.GetInKBps());
		m_NetSamplesOut.m_Data[ts].AddSample(netDataLine.GetOutKBps());
		break;
	}
	case ConsoleLineType::NetLatency:
	{
		auto& netLatencyLine = static_cast<const NetLatencyLine&>(parsed);
		auto ts = round_time_point(netLatencyLine.GetTimestamp(), 100ms);
		m_NetSamplesIn.m_Latency[ts].AddSample(netLatencyLine.GetInLatency());
		m_NetSamplesOut.m_Latency[ts].AddSample(netLatencyLine.GetOutLatency());
		break;
	}
	case ConsoleLineType::NetPacketsTotal:
	{
		auto& netPacketsLine = static_cast<const NetPacketsTotalLine&>(parsed);
		auto ts = round_time_point(netPacketsLine.GetTimestamp(), 100ms);
		m_NetSamplesIn.m_Packets[ts].AddSample(netPacketsLine.GetInPacketsPerSecond());
		m_NetSamplesOut.m_Packets[ts].AddSample(netPacketsLine.GetOutPacketsPerSecond());
		break;
	}
	case ConsoleLineType::NetLoss:
	{
		auto& netLossLine = static_cast<const NetLossLine&>(parsed);
		auto ts = round_time_point(netLossLine.GetTimestamp(), 100ms);
		m_NetSamplesIn.m_Loss[ts].AddSample(netLossLine.GetInLossPercent());
		m_NetSamplesOut.m_Loss[ts].AddSample(netLossLine.GetOutLossPercent());
		break;
	}
#endif

	default:
		break;
	}
}

Player& WorldState::FindOrCreatePlayer(const SteamID& id)
{
	Player* data;
	if (auto found = m_CurrentPlayerData.find(id); found != m_CurrentPlayerData.end())
	{
		data = found->second.get();
	}
	else
	{
		data = m_CurrentPlayerData.emplace(id, std::make_shared<Player>(*this, id)).first->second.get();

		if (!GetSettings().m_LazyLoadAPIData)
		{
			data->GetPlayerSummary();
			data->GetPlayerBans();
			data->GetTF2Playtime();
			data->GetLogsInfo();
			data->GetFriendsInfo();
			data->GetInventoryInfo();
			data->GetPlayerSourceBanState();
		}
	}


	assert(data->GetSteamID() == id);
	return *data;
}

auto WorldState::GetTeamShareResult(const SteamID& id0, const SteamID& id1) const -> TeamShareResult
{
	return GetTeamShareResult(FindLobbyMemberTeam(id0), FindLobbyMemberTeam(id1));
}

template<typename T>
static std::vector<SteamID> Take100(const T& collection)
{
	std::vector<SteamID> retVal;
	for (SteamID id : collection)
	{
		if (retVal.size() >= 100)
			break;

		retVal.push_back(id);
	}
	return retVal;
}

auto WorldState::PlayerSummaryUpdateAction::SendRequest(
	WorldState*& state, queue_collection_type& collection) -> response_future_type
{
	auto client = state->GetSettings().GetHTTPClient();
	if (!client)
		return {};

	if (!state->GetSettings().IsSteamAPIAvailable())
	{
		for (auto& entry : collection)
		{
			if (auto found = state->FindPlayer(entry))
				static_cast<Player*>(found)->m_PlayerSummary = SteamAPI::ErrorCode::SteamAPIDisabled;
		}
		return {};
	}

	std::vector<SteamID> steamIDs = Take100(collection);

	return SteamAPI::GetPlayerSummariesAsync(
		state->GetSettings(), std::move(steamIDs), *client);
}

void WorldState::PlayerSummaryUpdateAction::OnDataReady(WorldState*& state,
	const response_type& response, queue_collection_type& collection)
{
	DebugLog("[SteamAPI] Received {} player summaries", response.size());
	for (const SteamAPI::PlayerSummary& entry : response)
	{
		auto& player = state->FindOrCreatePlayer(entry.m_SteamID);
		player.m_PlayerSummary = entry;

		collection.erase(entry.m_SteamID);

		if (entry.m_CreationTime.has_value())
			state->m_AccountAges->OnDataReady(entry.m_SteamID, entry.m_CreationTime.value());
	}
}

auto WorldState::PlayerBansUpdateAction::SendRequest(state_type& state,
	queue_collection_type& collection) -> response_future_type
{
	auto client = state->GetSettings().GetHTTPClient();
	if (!client)
		return {};

	if (!state->GetSettings().IsSteamAPIAvailable())
	{
		for (auto& entry : collection)
		{
			if (auto found = state->FindPlayer(entry))
				static_cast<Player*>(found)->m_PlayerSteamBans = SteamAPI::ErrorCode::SteamAPIDisabled;
		}
		return {};
	}

	std::vector<SteamID> steamIDs = Take100(collection);
	return SteamAPI::GetPlayerBansAsync(
		state->GetSettings(), std::move(steamIDs), *client);
}

void WorldState::PlayerBansUpdateAction::OnDataReady(state_type& state,
	const response_type& response, queue_collection_type& collection)
{
	DebugLog("[SteamAPI] Received {} player bans", response.size());
	for (const SteamAPI::PlayerBans& bans : response)
	{
		state->FindOrCreatePlayer(bans.m_SteamID).m_PlayerSteamBans = bans;
		collection.erase(bans.m_SteamID);
	}
}

auto WorldState::PlayerSourceBansUpdateAction::SendRequest(state_type& state,
	queue_collection_type& collection) -> response_future_type
{
	auto client = state->GetSettings().GetHTTPClient();
	if (!client)
		return {};

	if (!state->GetSettings().m_AllowInternetUsage || !state->GetSettings().m_EnableSteamHistoryIntegration || state->GetSettings().GetSteamHistoryAPIKey().empty())
	{
		for (auto& entry : collection)
		{
			// TODO: make your own custom error... lol.. don't repurpose errors like this...
			if (auto found = state->FindPlayer(entry)) {
				static_cast<Player*>(found)->m_PlayerSourceBanState = ErrorCode::InternetConnectivityDisabled;
				static_cast<Player*>(found)->m_PlayerSourceBans = ErrorCode::InternetConnectivityDisabled;
			}

		}
		return {};
	}

	std::vector<SteamID> steamIDs = Take100(collection);

	return SteamHistoryAPI::GetPlayerSourceBansAsync(state->GetSettings().GetSteamHistoryAPIKey(), std::move(steamIDs), *client);
}

void WorldState::PlayerSourceBansUpdateAction::OnDataReady(state_type& state,
	const response_type& response, queue_collection_type& collection)
{
	DebugLog("[SteamHistory] Received {} player's bans", response.size());

	for (const auto& steamID : collection) {
		auto& player = state->FindOrCreatePlayer(steamID);
		// SteamHistoryAPI::PlayerSourceBans

		SteamHistoryAPI::PlayerSourceBanState banState;

		// we have a ban.
		if (response.find(steamID) != response.end()) {
			const auto& bans = response.at(steamID);

			DebugLog("[SteamHistory] user {} has {} ban records", steamID, bans.size());

			// set our entire history of bans (remove?)
			player.m_PlayerSourceBans = bans;

			// set our latest ban state for this user.
			for (const auto& ban : bans) {
				// we didn't store this server, or this ban is newer than the one we already stored.
				if (banState.find(ban.m_Server) == banState.end() || banState.at(ban.m_Server).m_BanTimestamp < ban.m_BanTimestamp) {
					banState.insert(std::pair(ban.m_Server, ban));
				}
			}

			player.m_PlayerSourceBanState = banState;
		}
		else {
			player.m_PlayerSourceBanState = banState;
			player.m_PlayerSourceBans = SteamHistoryAPI::PlayerSourceBans();
		}
	}

	// any other users are either errors (and we should reattempt)
	// or doesn't have a ban, so we can safely clear our queue.
	// FIXME: ask XVF so it returns keys at least for users with no bans
	collection.clear();
}
