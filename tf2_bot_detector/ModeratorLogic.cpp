#include "ModeratorLogic.h"
#include "Util/TextUtils.h"
#include "Actions/Actions.h"
#include "Actions/RCONActionManager.h"
#include "Config/PlayerListJSON.h"
#include "Config/Rules.h"
#include "Config/Settings.h"
#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/IConsoleLine.h"
#include "GameData/UserMessageType.h"
#include "GameData/IPlayer.h"
#include "Log.h"
#include "PlayerStatus.h"
#include "WorldEventListener.h"
#include "WorldState.h"
#include "Networking/SteamAPI.h"

#include <mh/algorithm/algorithm_generic.hpp>
#include <mh/algorithm/multi_compare.hpp>
#include <mh/text/case_insensitive_string.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/text/string_insertion.hpp>

#include <iomanip>
#include <map>
#include <regex>
#include <unordered_set>
#include <fstream>

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace
{
	class ModeratorLogic final : public IModeratorLogic, AutoConsoleLineListener, AutoWorldEventListener
	{
	public:
		ModeratorLogic(IWorldState& world, const Settings& settings, IRCONActionManager& actionManager);

		void Update() override;

		struct Cheater
		{
			Cheater(IPlayer& player, PlayerMarks marks) : m_Player(player), m_Marks(std::move(marks)) {}

			std::reference_wrapper<IPlayer> m_Player;
			PlayerMarks m_Marks;

			IPlayer* operator->() const { return &m_Player.get(); }
		};

		PlayerMarks GetPlayerAttributes(const SteamID& id) const override;
		PlayerMarks HasPlayerAttributes(const SteamID& id, const PlayerAttributesList& attributes, AttributePersistence persistence) const override;
		bool InitiateVotekick(const IPlayer& player, KickReason reason, const PlayerMarks* marks = nullptr) override;

		std::string GenerateCheaterWarnMessage(const std::vector<std::string>& names) const;

		bool SetPlayerAttribute(const IPlayer& id, PlayerAttribute markType, AttributePersistence persistence, bool set = true, std::string proof = "") override;
		bool SetPlayerAttribute(const SteamID& id, std::string name, PlayerAttribute markType, AttributePersistence persistence, bool set = true, std::string proof = "") override;

		std::optional<LobbyMemberTeam> TryGetMyTeam() const;
		TeamShareResult GetTeamShareResult(const SteamID& id) const override;
		const IPlayer* GetLocalPlayer() const;

		// Are we the leader of the bots? AKA do we have the lowest userID of
		// the pool of players we think are running the bot?
		bool IsBotLeader() const;
		const IPlayer* GetBotLeader() const override;

		duration_t TimeToConnectingCheaterWarning() const;
		duration_t TimeToCheaterWarning() const;

		bool IsUserRunningTool(const SteamID& id) const override;
		void SetUserRunningTool(const SteamID& id, bool isRunningTool = true) override;

		size_t GetBlacklistedPlayerCount() const override { return m_PlayerList.GetPlayerCount(); }
		size_t GetRuleCount() const override { return m_Rules.GetRuleCount(); }

		MarkedFriends GetMarkedFriendsCount(IPlayer& id) const override;

		void ReloadConfigFiles() override;

		PlayerListJSON* GetPlayerList() { return &m_PlayerList; }

	private:
		IWorldState* m_World = nullptr;
		const Settings* m_Settings = nullptr;
		IRCONActionManager* m_ActionManager = nullptr;

		struct PlayerExtraData
		{
			bool m_FriendsProcessed = false;
			MarkedFriends m_MarkedFriends;

			// If this is a known cheater, warn them ahead of time that the player is connecting, but only once
			// (we don't know the cheater's name yet, so don't spam if they can't do anything about it yet)
			bool m_PreWarnedOtherTeam = false;

			// same as above, but we don't want to spam party chat.
			bool m_PartyWarned = false;

			// If we're not the bot leader, prevent this player from triggering
			// any warnings (but still participates in other warnings!!!)
			std::optional<time_point_t> m_ConnectingWarningDelayEnd;
			std::optional<time_point_t> m_WarningDelayEnd;

			struct
			{
				time_point_t m_LastTransmission{};
				duration_t m_TotalTransmissions{};
			} m_Voice;
		};

		// Steam IDs of players that we think are running the tool.
		std::unordered_set<SteamID> m_PlayersRunningTool;

		void OnPlayerStatusUpdate(IWorldState& world, const IPlayer& player) override;
		void OnChatMsg(IWorldState& world, IPlayer& player, const std::string_view& msg) override;

		// FIXME: move to a different file, this really shouldn't be here.
		void OnLocalPlayerInitialized(IWorldState& world, bool initialized);

		void OnRuleMatch(const ModerationRule& rule, const IPlayer& player, std::string reason = "none");

		// How long inbetween accusations, unused as we pull from settings.
		static constexpr duration_t CHEATER_WARNING_INTERVAL = std::chrono::seconds(20);

		// How long inbetween accusations, for people that are not using a forked version of tf2bd.
		static constexpr duration_t CHEATER_WARNING_INTERVAL_DEFAULT = std::chrono::seconds(20);

		// The soonest we can make an accusation after having seen an accusation in chat from a bot leader.
		// This must be longer than CHEATER_WARNING_INTERVAL_DEFAULT.
		static constexpr duration_t CHEATER_WARNING_INTERVAL_NONLOCAL = CHEATER_WARNING_INTERVAL_DEFAULT + std::chrono::seconds(10);

		// How long we wait between determining someone is cheating and actually accusing them.
		// This delay exists to give a bot leader a chance to make an accusation.
		static constexpr duration_t CHEATER_WARNING_DELAY = std::chrono::seconds(10);

		time_point_t m_NextConnectingCheaterWarningTime{};  // The soonest we can warn about cheaters connecting to the server
		time_point_t m_NextCheaterWarningTime{};            // The soonest we can warn about connected cheaters on the other team
		time_point_t m_LastPlayerActionsUpdate{};
		void ProcessPlayerActions();
		void HandleFriendlyCheaters(uint8_t friendlyPlayerCount, uint8_t connectedFriendlyPlayerCount,
			const std::vector<Cheater>& friendlyCheaters);
		void HandleEnemyCheaters(uint8_t enemyPlayerCount, const std::vector<Cheater>& enemyCheaters,
			const std::vector<Cheater>& connectingEnemyCheaters);
		void HandleConnectedEnemyCheaters(const std::vector<Cheater>& enemyCheaters);
		void HandleConnectingEnemyCheaters(const std::vector<Cheater>& connectingEnemyCheaters);
		void HandleConnectingMarkedPlayers(const std::vector<Cheater>& connectingEnemyCheaters);

		// Minimum interval between callvote commands
		// this should be 150 seconds, which is the default of sv_vote_creation_timer;
		// however because we can't actually tell how many seconds left until votekick, i'm making it 75.
		static constexpr duration_t MIN_VOTEKICK_INTERVAL = std::chrono::seconds(75);
		time_point_t m_LastVoteCallTime{}; // Last time we called a votekick on someone
		duration_t GetTimeSinceLastCallVote() const { return tfbd_clock_t::now() - m_LastVoteCallTime; }

		/// <summary>
		/// can we call a votekick?
		/// </summary>
		/// <returns></returns>
		bool CanCallVoteKick() { return GetTimeSinceLastCallVote() > MIN_VOTEKICK_INTERVAL; }

		PlayerListJSON m_PlayerList;
		ModerationRules m_Rules;
	};

	template<typename CharT, typename Traits>
	static std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const ModeratorLogic::Cheater& cheater)
	{
		assert(cheater.m_Marks);
		os << cheater.m_Player.get() << " (marked in ";

		const auto markCount = cheater.m_Marks.m_Marks.size();
		for (size_t i = 0; i < markCount; i++)
		{
			const auto& mark = cheater.m_Marks.m_Marks[i];

			if (i != 0)
				os << ", ";

			if (i == (markCount - 1) && markCount > 1)
				os << "and ";

			os << std::quoted(mark.m_FileName);
		}

		os << ')';

		return os;
	}
}

// lazy and dumb function to make a string list of marked files
std::string listMarkFiles(PlayerMarks& marks) {
	PlayerAttributesList attribute{ 0 };
	std::vector<std::string> files;

	for (const auto& mark : marks.m_Marks)
	{
		// push file names into a nice vector so we can return
		if (std::find(files.begin(), files.end(), mark.m_FileName) != files.end()) {
			files.push_back(mark.m_FileName);
		}
	}

	std::ostringstream oss;
	std::copy(files.begin(), files.end() - 1, std::ostream_iterator<std::string>(oss, ","));
	oss << files.back();

	return oss.str();
}

// lazy and dumb function to convert player marks to string
std::string marksToString(PlayerMarks& marks) {
	PlayerAttributesList attribute{ 0 };

	// combine all the m_Marks to one attributeList
	for (const auto& mark : marks.m_Marks)
	{
		attribute |= mark.m_Attributes;
	}

	// only one attrib
	if (attribute.count() == 1) {
		std::size_t idx = 0;
		while (idx < static_cast<std::size_t>(PlayerAttribute::COUNT) && !attribute.HasAttribute(static_cast<PlayerAttribute>(idx))) { ++idx; }

		//if (idx == static_cast<std::size_t>(PlayerAttribute::COUNT)) {
		//	return "#Error!";
		//}

		return to_string(static_cast<PlayerAttribute>(idx));
	}

	std::string attrib_summary = "-----";

	// loop over the attributes and combine using first character.
	for (std::size_t i = 0; i < static_cast<std::size_t>(PlayerAttribute::COUNT); ++i) {
		if (attribute.HasAttribute(static_cast<PlayerAttribute>(i))) {
			attrib_summary.at(i) = to_string(static_cast<PlayerAttribute>(i)).at(0);
		}
	}

	return attrib_summary;
}

std::unique_ptr<IModeratorLogic> IModeratorLogic::Create(IWorldState& world,
	const Settings& settings, IRCONActionManager& actionManager)
{
	return std::make_unique<ModeratorLogic>(world, settings, actionManager);
}

void ModeratorLogic::Update()
{
	ProcessPlayerActions();
}

void ModeratorLogic::OnRuleMatch(const ModerationRule& rule, const IPlayer& player, std::string reason)
{
	for (PlayerAttribute attribute : rule.m_Actions.m_Mark)
	{
		if (SetPlayerAttribute(player, attribute, AttributePersistence::Saved, true, "[auto] automatically marked: " + to_string(attribute) + " | reason: " + reason))
			Log("Marked {} with {:v} due to rule match with {}", player, mh::enum_fmt(attribute), std::quoted(rule.m_Description));
	}
	for (PlayerAttribute attribute : rule.m_Actions.m_TransientMark)
	{
		if (SetPlayerAttribute(player, attribute, AttributePersistence::Transient))
			Log("[TRANSIENT] Marked {} with {:v} due to rule match with {}", player, mh::enum_fmt(attribute), std::quoted(rule.m_Description));
	}
	for (PlayerAttribute attribute : rule.m_Actions.m_Unmark)
	{
		if (SetPlayerAttribute(player, attribute, AttributePersistence::Saved, false))
			Log("Unmarked {} with {:v} due to rule match with {}", player, mh::enum_fmt(attribute), std::quoted(rule.m_Description));
	}
}

void ModeratorLogic::OnPlayerStatusUpdate(IWorldState& world, const IPlayer& player)
{
	const auto name = player.GetNameUnsafe();
	const auto steamID = player.GetSteamID();

	if (m_Settings->m_AutoMark)
	{
		for (const ModerationRule& rule : m_Rules.GetRules())
		{
			if (!rule.Match(player))
				continue;

			OnRuleMatch(rule, player, rule.m_Description);
		}
	}
}

static bool IsBotDetectorMessage(const std::string_view& msg)
{
	static const std::regex s_IngameWarning(
		R"regex(Attention! There (?:is a|are \d+) cheaters? on the other team named .*\. Please kick them!)regex",
		std::regex::optimize);
	static const std::regex s_ConnectingWarning(
		R"regex(Heads up! There (?:is a|are \d+) known cheaters? joining the other team! Names? unknown until they fully join\.)regex",
		std::regex::optimize);

	return std::regex_match(msg.begin(), msg.end(), s_IngameWarning) || std::regex_match(msg.begin(), msg.end(), s_ConnectingWarning);
}

void ModeratorLogic::OnChatMsg(IWorldState& world, IPlayer& player, const std::string_view& msg)
{
	bool botMsgDetected = IsBotDetectorMessage(msg);

	// Check if it is a moderation message from someone else
	if (m_Settings->m_AutoTempMute &&
		!m_PlayerList.HasPlayerAttributes(player, { PlayerAttribute::Cheater, PlayerAttribute::Exploiter }))
	{
		if (auto localPlayer = GetLocalPlayer(); localPlayer && (player.GetSteamID() != localPlayer->GetSteamID()) && botMsgDetected)
		{
			Log("Detected message from {} as another instance of TF2BD: {}", player, std::quoted(msg));
			SetUserRunningTool(player, true);

			if (player.GetUserID() < localPlayer->GetUserID())
			{
				assert(!IsBotLeader());
				Log("Deferring all cheater warnings for a bit because we think {} is running TF2BD", player);
				m_NextCheaterWarningTime = m_NextConnectingCheaterWarningTime =
					world.GetCurrentTime() + CHEATER_WARNING_INTERVAL_NONLOCAL;
			}
		}
	}

	if (m_Settings->m_AutoMark && !botMsgDetected)
	{
		for (const ModerationRule& rule : m_Rules.GetRules())
		{
			if (!rule.Match(player, msg))
				continue;

			// why must i do this this feels dumb
			std::ostringstream os;
			os << std::quoted(msg);

			OnRuleMatch(rule, player, os.str());
			Log("Chat message rule match for {}: {}", rule.m_Description, os.str());
		}
	}
}

void ModeratorLogic::OnLocalPlayerInitialized(IWorldState & world, bool initialized)
{
	m_ActionManager->QueueAction<GenericCommandAction>("exec tf2bd/OnGameJoin");

	// do our "onjoin" message
	if (m_Settings->m_AutoChatWarningsConnectingParty && m_Settings->m_AutoChatWarningsConnectingPartyPrintSummary) {
		mh::fmtstr<128> chatMsg;

		int markedPlayerCount = 0;
		std::vector<tf2_bot_detector::string> players;

		for (IPlayer& player : m_World->GetLobbyMembers())
		{
			if (!m_PlayerList.GetPlayerAttributes(player).empty()) {

				auto marks = GetPlayerAttributes(player);

				tf2_bot_detector::string username = player.GetNameSafe();

				// if username is none try getting data from steamapi
				if (username == "") {
					auto summary = player.GetPlayerSummary();

					
					if (summary.has_value()) {
						username = summary.value().m_Nickname;
					}
					else {
						// steamapi didn't get the name either so gg
						username = player.GetSteamID().str();
					}
				}

				players.push_back(
					mh::fmtstr<128>(
						"{}: {} - {} ({})",
						markedPlayerCount,
						username,
						marksToString(marks),
						marks.m_Marks.front().m_FileName
					).c_str()
				);
				++markedPlayerCount;

				// don't warn again, we've already warned about this here.
				player.GetOrCreateData<PlayerExtraData>().m_PartyWarned = true;
			}
		}

		chatMsg.fmt("[tf2bd] Currently {} players are marked in this lobby.", markedPlayerCount);

		m_ActionManager->QueueAction<PartyChatMessageAction>(chatMsg.str());

		for (std::string player : players) {
			m_ActionManager->QueueAction<PartyChatMessageAction>(player);
		}
	}
}

static bool CanPassVote(size_t totalPlayerCount, size_t cheaterCount, float* cheaterRatio = nullptr)
{
	if (cheaterRatio)
		*cheaterRatio = float(cheaterCount) / totalPlayerCount;

	return (cheaterCount * 2) < totalPlayerCount;
}

void ModeratorLogic::HandleFriendlyCheaters(uint8_t friendlyPlayerCount, uint8_t connectedFriendlyPlayerCount,
	const std::vector<Cheater>& friendlyCheaters)
{
	if (!m_Settings->m_AutoVotekick)
		return;

	if (friendlyCheaters.empty())
		return; // Nothing to do

	constexpr float MIN_QUORUM = 0.61f;
	if (auto quorum = float(connectedFriendlyPlayerCount) / friendlyPlayerCount; quorum <= MIN_QUORUM)
	{
		LogWarning("Impossible to pass a successful votekick against "s << friendlyCheaters.size()
			<< " friendly cheaters: only " << int(quorum * 100)
			<< "% of players on our team are connected right now (need >= " << int(MIN_QUORUM * 100) << "%)");
		return;
	}

	if (float cheaterRatio; !CanPassVote(friendlyPlayerCount, friendlyCheaters.size(), &cheaterRatio))
	{
		LogWarning("Impossible to pass a successful votekick against "s << friendlyCheaters.size()
			<< " friendly cheaters: our team is " << int(cheaterRatio * 100) << "% cheaters");
		return;
	}

	// Votekick the first one that is actually connected
	for (const Cheater& cheater : friendlyCheaters)
	{
		if (cheater->GetSteamID() == m_Settings->GetLocalSteamID())
			continue;

		if (cheater->GetConnectionState() == PlayerStatusState::Active)
		{
			if (InitiateVotekick(cheater.m_Player, KickReason::Cheating, &cheater.m_Marks))
				break;
		}
	}
}

template<typename TIter>
static std::vector<std::string> GetJoinedStrings(const TIter& begin, const TIter& end, const std::string_view& separator)
{
	std::vector<std::string> retVal;
	std::string strBuf;
	for (auto it = begin; it != end; ++it)
	{
		if (it != begin)
			strBuf << separator;

		strBuf << *it;
		retVal.push_back(strBuf);
	}

	return retVal;
}

void ModeratorLogic::HandleEnemyCheaters(uint8_t enemyPlayerCount,
	const std::vector<Cheater>& enemyCheaters, const std::vector<Cheater>& connectingEnemyCheaters)
{
	if (enemyCheaters.empty() && connectingEnemyCheaters.empty())
		return;

	const auto cheaterCount = enemyCheaters.size() + connectingEnemyCheaters.size();
	if (float cheaterRatio; !CanPassVote(enemyPlayerCount, cheaterCount, &cheaterRatio))
	{
		LogWarning("Impossible to pass a successful votekick against "s << cheaterCount
			<< " enemy cheaters (enemy team is " << int(cheaterRatio * 100) << "% cheaters). Skipping all warnings.");
		return;
	}

	if (!enemyCheaters.empty())
		HandleConnectedEnemyCheaters(enemyCheaters);
	else if (!connectingEnemyCheaters.empty())
		HandleConnectingEnemyCheaters(connectingEnemyCheaters);
}

void ModeratorLogic::HandleConnectedEnemyCheaters(const std::vector<Cheater>& enemyCheaters)
{
	const auto now = tfbd_clock_t::now();

	// There are enough people on the other team to votekick the cheater(s)
	std::string logMsg = mh::format("Telling the other team about {} cheater(s) named ", enemyCheaters.size());

	const bool isBotLeader = IsBotLeader();
	bool needsWarning = false;
	std::vector<std::string> chatMsgCheaterNames;
	std::multimap<std::string, Cheater> cheaterDebugWarnings;
	for (auto& cheater : enemyCheaters)
	{
		if (cheater->GetNameSafe().empty())
			continue; // Theoretically this should never happen, but don't embarass ourselves

		mh::format_to(std::back_inserter(logMsg), "\n\t{}", cheater);
		chatMsgCheaterNames.emplace_back(cheater->GetNameSafe());

		auto& cheaterData = cheater->GetOrCreateData<PlayerExtraData>();

		if (isBotLeader)
		{
			// We're supposedly in charge
			cheaterDebugWarnings.emplace("We're bot leader: Triggered ACTIVE warning for ", cheater);
			needsWarning = true;
		}
		else if (cheaterData.m_WarningDelayEnd.has_value())
		{
			if (now >= cheaterData.m_WarningDelayEnd)
			{
				cheaterDebugWarnings.emplace("We're not bot leader: Delay expired for ACTIVE cheater(s) ", cheater);
				needsWarning = true;
			}
			else
			{
				cheaterDebugWarnings.emplace(
					mh::format("We're not bot leader: {} seconds remaining for ACTIVE cheater(s) ", to_seconds(cheaterData.m_WarningDelayEnd.value() - now)),
					cheater);
			}
		}
		else if (!cheaterData.m_WarningDelayEnd.has_value())
		{
			cheaterDebugWarnings.emplace("We're not bot leader: Starting delay for ACTIVE cheater(s) ", cheater);
			cheaterData.m_WarningDelayEnd = now + CHEATER_WARNING_DELAY;
		}
	}

	mh::for_each_multimap_group(cheaterDebugWarnings,
		[&](const std::string_view& msgBase, auto cheatersBegin, auto cheatersEnd)
		{
			mh::fmtstr<2048> msgFmt;
			msgFmt.puts(msgBase);

			for (auto it = cheatersBegin; it != cheatersEnd; ++it)
			{
				if (it != cheatersBegin)
					msgFmt.puts(", ");

				msgFmt.fmt("{}", it->second);
			}
		});

	if (!needsWarning)
		return;

	if (chatMsgCheaterNames.size() > 0)
	{
		if (now >= m_NextCheaterWarningTime)
		{
			if (m_Settings->m_AutoChatWarnings && m_ActionManager->QueueAction<ChatMessageAction>(GenerateCheaterWarnMessage(chatMsgCheaterNames)))
			{
				Log({ 1, 0, 0, 1 }, logMsg);
				// used to be CHEATER_WARNING_INTERVAL
				m_NextCheaterWarningTime = now + std::chrono::seconds(m_Settings->m_ChatWarningInterval);
			}
		}
		else
		{
			DebugLog("HandleEnemyCheaters(): Skipping cheater warnings for "s << to_seconds(m_NextCheaterWarningTime - now) << " seconds");
		}
	}
}

std::string ModeratorLogic::GenerateCheaterWarnMessage(const std::vector<std::string>& names) const
{
	// TODO: have the defauts in a const somewhere else idk
	std::string one_cheater_warning = "Attention! There is a cheater on the other team named \"{}\". Please kick them!";
	std::string multiple_cheater_warning = "Attention! There are {} cheaters on the other team named {}. Please kick them!";

	if (m_Settings->m_UseCustomChatWarnings) {
		// check if they're empty for whatever reason
		if (!m_Settings->m_OneCheaterWarningMessage.empty()) {
			try {
				fmt::format(m_Settings->m_OneCheaterWarningMessage, 1);
				one_cheater_warning = m_Settings->m_OneCheaterWarningMessage;
			}
			catch (fmt::format_error err) {
				LogError("Our custom one cheater warning message is invalid; falling back to default.");
			}
		}

		if (!m_Settings->m_MultipleCheaterWarningMessage.empty()) {
			try {
				fmt::format(m_Settings->m_MultipleCheaterWarningMessage, fmt::arg("count", 2), fmt::arg("names", "a, b"));
				multiple_cheater_warning = m_Settings->m_MultipleCheaterWarningMessage;
			}
			catch (fmt::format_error err) {
				LogError("Our custom mutiple cheater warning message is invalid; falling back to default.");
			}
		}
	}

	constexpr size_t MAX_CHATMSG_LENGTH = 127;

	size_t max_names_length_one = MAX_CHATMSG_LENGTH - one_cheater_warning.size() - 1 - 2;
	size_t max_names_length_multiple = MAX_CHATMSG_LENGTH - multiple_cheater_warning.size() - 1 - 1 - 2;

	assert(max_names_length_one >= 32);

	mh::fmtstr<MAX_CHATMSG_LENGTH + 1> chatMsg;

	if (names.size() == 1)
	{
		chatMsg.fmt(one_cheater_warning, names.front());
	}
	else
	{
		assert(names.size() > 0);
		std::string cheaters;

		for (std::string cheaterNameStr : GetJoinedStrings(names.begin(), names.end(), ", "sv))
		{
			if (cheaters.empty() || cheaterNameStr.size() <= max_names_length_multiple)
				cheaters = std::move(cheaterNameStr);
			else if (cheaterNameStr.size() > max_names_length_multiple)
				break;
		}

		chatMsg.fmt(multiple_cheater_warning, fmt::arg("count", names.size()), fmt::arg("names", cheaters));
	}

	assert(chatMsg.size() <= 127);

	return chatMsg.str();
}

void ModeratorLogic::HandleConnectingEnemyCheaters(const std::vector<Cheater>& connectingEnemyCheaters)
{
	if (!m_Settings->m_AutoChatWarnings || !m_Settings->m_AutoChatWarningsConnecting)
		return;  // user has disabled this functionality

	const auto now = tfbd_clock_t::now();
	if (now < m_NextConnectingCheaterWarningTime)
	{
		DebugLog("HandleEnemyCheaters(): Discarding connection warnings ("s
			<< to_seconds(m_NextConnectingCheaterWarningTime - now) << " seconds left)");

		// Assume someone else with a lower userid is in charge, discard warnings about
		// connecting enemy cheaters while it looks like they are doing stuff
		for (auto cheater : connectingEnemyCheaters)
			cheater->GetOrCreateData<PlayerExtraData>().m_PreWarnedOtherTeam = true;

		return;
	}

	const bool isBotLeader = IsBotLeader();
	bool needsWarning = false;
	for (auto& cheater : connectingEnemyCheaters)
	{
		auto& cheaterData = cheater->GetOrCreateData<PlayerExtraData>();
		if (cheaterData.m_PreWarnedOtherTeam)
			continue; // Already included in a warning

		if (isBotLeader)
		{
			// We're supposedly in charge
			DebugLog("We're bot leader: Triggered connecting warning for {}", cheater);
			needsWarning = true;
			break;
		}
		else if (cheaterData.m_ConnectingWarningDelayEnd.has_value())
		{
			if (now >= cheaterData.m_ConnectingWarningDelayEnd)
			{
				DebugLog("We're not bot leader: Delay expired for connecting cheater {}", cheater);
				needsWarning = true;
				break;
			}
			else
			{
				DebugLog("We're not bot leader: {} seconds remaining for connecting cheater {}",
					to_seconds(cheaterData.m_ConnectingWarningDelayEnd.value() - now), cheater);
			}
		}
		else if (!cheaterData.m_ConnectingWarningDelayEnd.has_value())
		{
			DebugLog("We're not bot leader: Starting delay for connecting cheater {}", cheater);
			cheaterData.m_ConnectingWarningDelayEnd = now + CHEATER_WARNING_DELAY;
		}
	}

	if (!needsWarning)
		return;

	mh::fmtstr<128> chatMsg;
	if (connectingEnemyCheaters.size() == 1)
	{
		if (m_Settings->m_UseCustomChatWarnings && !m_Settings->m_OneCheaterConnectingMessage.empty()) {
			chatMsg.puts(m_Settings->m_OneCheaterConnectingMessage);
		}
		else {
			// move this string to a const somewhere else idk
			chatMsg.puts("Heads up! There is a known cheater joining the other team! Name unknown until they fully join.");
		}
	}
	else
	{
		// move this string to a const somewhere else idk
		std::string mutli_cheater_connecting = "Heads up! There are {} known cheaters joining the other team! Names unknown until they fully join.";

		if (m_Settings->m_UseCustomChatWarnings && !m_Settings->m_MultipleCheaterConnectingMessage.empty()) {
			try {
				fmt::format(m_Settings->m_MultipleCheaterConnectingMessage, 1);
				mutli_cheater_connecting = m_Settings->m_MultipleCheaterConnectingMessage;
			}
			catch (fmt::format_error err) {
				LogError("Our multi-cheater connecting message is invalid; falling back to default.");
			}
		}

		chatMsg.fmt(mutli_cheater_connecting, connectingEnemyCheaters.size());
	}

	Log("Telling other team about "s << connectingEnemyCheaters.size() << " cheaters currently connecting");
	if (m_ActionManager->QueueAction<ChatMessageAction>(chatMsg))
	{
		for (auto& cheater : connectingEnemyCheaters)
			cheater->GetOrCreateData<PlayerExtraData>().m_PreWarnedOtherTeam = true;
	}
}

void ModeratorLogic::HandleConnectingMarkedPlayers(const std::vector<Cheater>& connectingEnemyCheaters)
{
	// we either disabled it or dont have anything in the vector
	if (!m_Settings->m_AutoChatWarningsConnectingParty || connectingEnemyCheaters.size() < 1) {
		return;
	}

	// seperate unwarned cheater data with already warned ones
	std::vector<Cheater> unwarnedCheaters;
	for (auto& p : connectingEnemyCheaters) {
		if (!p->GetOrCreateData<PlayerExtraData>().m_PartyWarned) {
			unwarnedCheaters.push_back(p);
		}
	}

	// we don't have new cheaters to warn about
	if (unwarnedCheaters.size() < 1) {
		return;
	}

	mh::fmtstr<128> chatMsg;

	if (unwarnedCheaters.size() == 1)
	{
		auto& cheaterData = unwarnedCheaters.at(0)->GetOrCreateData<PlayerExtraData>();
		if (cheaterData.m_PartyWarned)
			return;

		tf2_bot_detector::IPlayer& player = unwarnedCheaters.at(0).m_Player.get();
		PlayerMarks marks = unwarnedCheaters.at(0).m_Marks;
		SteamID steamid = player.GetSteamID();

		// this looks ugly, but realistically you shouldn't be using this software with steamapi disabled.
		std::string username = "";

		// attempt to get a "true" username from steamapi, if enabled.
		if (m_Settings->IsSteamAPIAvailable()) {
			auto summary = player.GetPlayerSummary();

			// steamapi didn't get the name yet, exit the function and this function will run again next loop.
			if (!summary.has_value()) {
				Log(steamid.str() + " - steamapi didnt recieve info, waiting until we receve data for this player.");
				return;
			}

			username = (summary.value().m_Nickname);
		}

		// move this into a func
		size_t pos;
		while ((pos = username.find(";")) != std::string::npos) {
			username.replace(pos, 1, "");
		}

		// TODO: cite multiple files?
		tf2_bot_detector::ConfigFileName fileName = marks.m_Marks.front().m_FileName;

		if (std::filesystem::exists(fileName)) {
			fileName = fileName.substr(fileName.rfind("playerlist."));
		}

		chatMsg.fmt("[tf2bd] WARN: Marked Player ({}) Joining ({} - {}).", username, marksToString(marks), fileName);
	}
	else
	{
		std::string msg = "";

		for (auto& p : unwarnedCheaters) {
			auto& cheaterData = p->GetOrCreateData<PlayerExtraData>();
			if (cheaterData.m_PartyWarned)
				continue;

			tf2_bot_detector::IPlayer& player = p.m_Player.get();
			PlayerMarks marks = p.m_Marks;
			SteamID steamid = player.GetSteamID();

			// this looks ugly, but realistically you shouldn't be using this software with steamapi disabled.
			std::string name = "";

			// attempt to get a "true" username from steamapi, if enabled.
			if (m_Settings->IsSteamAPIAvailable()) {
				auto summary = player.GetPlayerSummary();

				// steamapi didn't get the name yet, exit the function and this function will run again next loop.
				if (!summary.has_value()) {
					Log(steamid.str() + " - steamapi didnt recieve info, waiting until we receve data for this player." );
					return;
				}

				name = (summary.value().m_Nickname);
			}

			// sanitize our name; 
			// move this into a func
			size_t pos;
			while ((pos = name.find(";")) != std::string::npos) {
				name.replace(pos, 1, "");
			}

			if (name.size() > 10) {
				name.resize(8, '.');
				name += "..";
			}

			// TODO: cite multiple files?
			tf2_bot_detector::ConfigFileName fileName = marks.m_Marks.front().m_FileName;

			if (std::filesystem::exists(fileName)) {
				fileName = fileName.substr(fileName.rfind("playerlist."));
			}

			msg += mh::format("{} - {}, ", name, marksToString(marks), fileName);
		}

		msg.pop_back();
		msg.pop_back();

		chatMsg.fmt("[tf2bd] WARN: {} Marked Players Joining. ({})", connectingEnemyCheaters.size(), msg);
	}

	if (m_ActionManager->QueueAction<PartyChatMessageAction>(chatMsg.str()))
	{
		for (auto& cheater : unwarnedCheaters)
			cheater->GetOrCreateData<PlayerExtraData>().m_PartyWarned = true;
	}
}


void ModeratorLogic::ProcessPlayerActions()
{
	const auto now = m_World->GetCurrentTime();
	if ((now - m_LastPlayerActionsUpdate) < 1s)
	{
		return;
	}
	else
	{
		m_LastPlayerActionsUpdate = now;
	}

	if (auto self = m_World->FindPlayer(m_Settings->GetLocalSteamID());
		(self && self->GetConnectionState() != PlayerStatusState::Active) ||
		!m_World->IsLocalPlayerInitialized())
	{
		DebugLog("Skipping ProcessPlayerActions() because we are not fully connected yet");
		return;
	}

	// Don't process actions if we're way out of date
	[[maybe_unused]] const auto dbgDeltaTime = to_seconds(tfbd_clock_t::now() - now);
	if ((tfbd_clock_t::now() - now) > 15s)
		return;

	const auto myTeam = TryGetMyTeam();
	if (!myTeam)
		return; // We don't know what team we're on, so we can't really take any actions.

	uint8_t totalEnemyPlayers = 0;
	uint8_t connectedEnemyPlayers = 0;
	uint8_t totalFriendlyPlayers = 0;
	uint8_t connectedFriendlyPlayers = 0;
	std::vector<Cheater> enemyCheaters;
	std::vector<Cheater> friendlyCheaters;
	std::vector<Cheater> connectingEnemyCheaters;
	// the struct Cheater doesnt really have to be always a cheater.
	std::vector<Cheater> connectingMarkedPlayer;

	const bool isBotLeader = IsBotLeader();
	bool needsEnemyWarning = false;
	for (IPlayer& player : m_World->GetLobbyMembers())
	{
		const bool isPlayerConnected = player.GetConnectionState() == PlayerStatusState::Active;
		const auto isCheater = m_PlayerList.HasPlayerAttributes(player, PlayerAttribute::Cheater);
		const bool isMarked = !m_PlayerList.GetPlayerAttributes(player).empty();
		const auto teamShareResult = m_World->GetTeamShareResult(*myTeam, player);

		if (isMarked && !isPlayerConnected)
		{
			connectingMarkedPlayer.push_back({ player, m_PlayerList.GetPlayerAttributes(player) });
		}

		if (teamShareResult == TeamShareResult::SameTeams)
		{
			if (isPlayerConnected)
			{
				if (player.GetActiveTime() > m_Settings->GetAutoVotekickDelay())
					connectedFriendlyPlayers++;

				if (!!isCheater)
					friendlyCheaters.push_back({ player, isCheater });
			}

			totalFriendlyPlayers++;
		}
		else if (teamShareResult == TeamShareResult::OppositeTeams)
		{
			if (isPlayerConnected)
			{
				connectedEnemyPlayers++;

				if (isCheater && !player.GetNameSafe().empty())
					enemyCheaters.push_back({ player, isCheater });
			}
			else
			{
				if (isCheater)
					connectingEnemyCheaters.push_back({ player, isCheater });
			}

			totalEnemyPlayers++;
		}
	}

	HandleEnemyCheaters(totalEnemyPlayers, enemyCheaters, connectingEnemyCheaters);
	HandleFriendlyCheaters(totalFriendlyPlayers, connectedFriendlyPlayers, friendlyCheaters);
	HandleConnectingMarkedPlayers(connectingMarkedPlayer);
}

bool ModeratorLogic::SetPlayerAttribute(const IPlayer& player, PlayerAttribute attribute, AttributePersistence persistence, bool set, std::string proof)
{
	return SetPlayerAttribute(player.GetSteamID(), player.GetNameUnsafe(), attribute, persistence, set, proof);
}

bool ModeratorLogic::SetPlayerAttribute(const SteamID& player, std::string name, PlayerAttribute attribute, AttributePersistence persistence, bool set, std::string proof)
{
	bool attributeChanged = false;

	m_PlayerList.ModifyPlayer(player, [&](PlayerListData& data)
		{
			PlayerAttributesList& attribs = [&]() -> PlayerAttributesList&
			{
				switch (persistence)
				{
				case AttributePersistence::Any:
					LogError("Invalid AttributePersistence {}", mh::enum_fmt(persistence));
					[[fallthrough]];
				case AttributePersistence::Saved:      return data.m_SavedAttributes;
				case AttributePersistence::Transient:  return data.m_TransientAttributes;
				}

				throw std::invalid_argument(mh::format("{}", MH_SOURCE_LOCATION_CURRENT()));
			}();

			attributeChanged = attribs.SetAttribute(attribute, set);

			if (!data.m_LastSeen)
				data.m_LastSeen.emplace();

			data.m_LastSeen->m_Time = m_World->GetCurrentTime();

			if (!name.empty())
				data.m_LastSeen->m_PlayerName = name;

			if (proof != "" && !data.proofExists(proof)) {
				data.addProof(proof);
			}

			return ModifyPlayerAction::Modified;
		});

	return attributeChanged;
}

std::optional<LobbyMemberTeam> ModeratorLogic::TryGetMyTeam() const
{
	return m_World->FindLobbyMemberTeam(m_Settings->GetLocalSteamID());
}

TeamShareResult ModeratorLogic::GetTeamShareResult(const SteamID& id) const
{
	return m_World->GetTeamShareResult(id);
}

const IPlayer* ModeratorLogic::GetLocalPlayer() const
{
	return m_World->FindPlayer(m_Settings->GetLocalSteamID());
}

bool ModeratorLogic::IsBotLeader() const
{
	auto leader = GetBotLeader();
	return leader && (leader->GetSteamID() == m_Settings->GetLocalSteamID());
}

const IPlayer* ModeratorLogic::GetBotLeader() const
{
	auto localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return nullptr;

	UserID_t localUserID;
	if (auto id = localPlayer->GetUserID())
		localUserID = *id;
	else
		return nullptr;

	const auto now = m_World->GetCurrentTime();
	for (const IPlayer& player : m_World->GetPlayers())
	{
		if (player.GetTimeSinceLastStatusUpdate() > 20s)
			continue;

		if (player.GetSteamID() == localPlayer->GetSteamID())
			continue;

		if (player.GetUserID() >= localUserID)
			continue;

		if (IsUserRunningTool(player))
			return &player;
	}

	return localPlayer;
}

duration_t ModeratorLogic::TimeToConnectingCheaterWarning() const
{
	return m_NextConnectingCheaterWarningTime - m_World->GetCurrentTime();
}

duration_t ModeratorLogic::TimeToCheaterWarning() const
{
	return m_NextCheaterWarningTime - m_World->GetCurrentTime();
}

bool ModeratorLogic::IsUserRunningTool(const SteamID& id) const
{
	return m_PlayersRunningTool.contains(id);
}
void ModeratorLogic::SetUserRunningTool(const SteamID& id, bool isRunningTool)
{
	if (isRunningTool)
		m_PlayersRunningTool.insert(id);
	else
		m_PlayersRunningTool.erase(id);
}

// why is this "unordered_map<PlayerAttribute, uint32_t>" ?
// in the event that i make PlayerAttribute more flexible or something idk
// this code isnt ready though LMAO
MarkedFriends ModeratorLogic::GetMarkedFriendsCount(IPlayer& player) const
{
	auto& data = player.GetOrCreateData<PlayerExtraData>();
	if (data.m_FriendsProcessed) {
		return data.m_MarkedFriends;
	}
	
	auto friendsInfo = player.GetFriendsInfo();

	// steamapi didn't get friends data yet; exit the function and this function will run again next loop.
	if (!friendsInfo.has_value()) {
		Log(player.GetSteamID().str() + " waiting until we receive friends list data for this player.");
		return data.m_MarkedFriends;
	}

	uint32_t totalCount = 0;
	uint32_t cheaterCount = 0;
	uint32_t suspiciousCount = 0;
	uint32_t exploiterCount = 0;
	uint32_t racistCount = 0;

	for (const SteamID& id : friendsInfo.value().m_Friends) {
		auto playerAttributes = GetPlayerAttributes(id);

		if (!playerAttributes.empty())
		{
			// TODO: dont do this ig
			if (playerAttributes.Has(PlayerAttribute::Cheater))
				cheaterCount++;
			if (playerAttributes.Has(PlayerAttribute::Suspicious))
				suspiciousCount++;
			if (playerAttributes.Has(PlayerAttribute::Exploiter))
				exploiterCount++;
			if (playerAttributes.Has(PlayerAttribute::Racist))
				racistCount++;

			totalCount++;
		}
	}

	data.m_MarkedFriends.m_FriendsCountTotal = static_cast<uint32_t>(friendsInfo.value().m_Friends.size());

	data.m_MarkedFriends.m_MarkedFriendsCount.insert(std::pair<PlayerAttribute, uint32_t>(PlayerAttribute::Cheater, cheaterCount));
	data.m_MarkedFriends.m_MarkedFriendsCount.insert(std::pair<PlayerAttribute, uint32_t>(PlayerAttribute::Suspicious, suspiciousCount));
	data.m_MarkedFriends.m_MarkedFriendsCount.insert(std::pair<PlayerAttribute, uint32_t>(PlayerAttribute::Exploiter, exploiterCount));
	data.m_MarkedFriends.m_MarkedFriendsCount.insert(std::pair<PlayerAttribute, uint32_t>(PlayerAttribute::Racist, racistCount));
	data.m_MarkedFriends.m_MarkedFriendsCountTotal = totalCount;
	data.m_FriendsProcessed = true;

	return data.m_MarkedFriends;
}

void ModeratorLogic::ReloadConfigFiles()
{
	m_PlayerList.LoadFiles();
	m_Rules.LoadFiles();
}

ModeratorLogic::ModeratorLogic(IWorldState& world, const Settings& settings, IRCONActionManager& actionManager) :
	AutoConsoleLineListener(world),
	AutoWorldEventListener(world),
	m_World(&world),
	m_Settings(&settings),
	m_ActionManager(&actionManager),
	m_PlayerList(settings),
	m_Rules(settings)
{
}

PlayerMarks ModeratorLogic::GetPlayerAttributes(const SteamID& id) const
{
	return m_PlayerList.GetPlayerAttributes(id);
}

PlayerMarks ModeratorLogic::HasPlayerAttributes(const SteamID& id, const PlayerAttributesList& attributes, AttributePersistence persistence) const
{
	return m_PlayerList.HasPlayerAttributes(id, attributes, persistence);
}

bool ModeratorLogic::InitiateVotekick(const IPlayer& player, KickReason reason, const PlayerMarks* marks)
{
	const auto userID = player.GetUserID();
	if (!userID)
	{
		Log("Wanted to kick {}, but could not find userid", player);
		return false;
	}

	// wait until we can call a votekick.
	if (CanCallVoteKick()) {
		return false;
	}

	if (m_ActionManager->QueueAction<KickAction>(userID.value(), reason))
	{
		std::string logMsg = mh::format("InitiateVotekick on {}: {:v}", player, mh::enum_fmt(reason));
		if (marks)
			mh::format_to_container(logMsg, ", in playerlist(s){}", *marks);

		Log(std::move(logMsg));

		m_LastVoteCallTime = tfbd_clock_t::now();
	}

	//m_ActionManager->QueueAction<ChatMessageAction>("[tf2bd] votekicking ", ChatMessageType::PartyChat);

	return true;
}

duration_t VoteCooldown::GetRemainingDuration() const
{
	return m_Total - m_Elapsed;
}

float VoteCooldown::GetProgress() const
{
	if (m_Elapsed >= m_Total)
		return 1;

	return static_cast<float>(to_seconds(m_Elapsed) / to_seconds(m_Total));
}
