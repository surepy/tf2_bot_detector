#include "Application.h"
#include "Config/Settings.h"
#include "GameData/MatchmakingQueue.h"
#include "GameData/UserMessageType.h"
#include "UI/MainWindow.h"
#include "UI/ImGui_TF2BotDetector.h"
#include "Util/RegexUtils.h"
#include "Log.h"
#include "WorldState.h"

#include <mh/algorithm/multi_compare.hpp>
#include <mh/text/charconv_helper.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/text/format.hpp>
#include <mh/text/string_insertion.hpp>
#include <ScopeGuards.h>

#include <regex>
#include <sstream>
#include <stdexcept>

#include "DifferingLobbyReceivedLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

DifferingLobbyReceivedLine::DifferingLobbyReceivedLine(time_point_t timestamp, const Lobby& newLobby,
	const Lobby& currentLobby, bool connectedToMatchServer, bool hasLobby, bool assignedMatchEnded) :
	BaseClass(timestamp), m_NewLobby(newLobby), m_CurrentLobby(currentLobby),
	m_ConnectedToMatchServer(connectedToMatchServer), m_HasLobby(hasLobby), m_AssignedMatchEnded(assignedMatchEnded)
{
}

std::shared_ptr<IConsoleLine> DifferingLobbyReceivedLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(
		R"regex(Differing lobby received\. Lobby: (.*)\/Match(\d+)\/Lobby(\d+) CurrentlyAssigned: (.*)\/Match(\d+)\/Lobby(\d+) ConnectedToMatchServer: (\d+) HasLobby: (\d+) AssignedMatchEnded: (\d+))regex",
		std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		Lobby newLobby;
		newLobby.m_LobbyID = SteamID(result[1].str());
		from_chars_throw(result[2], newLobby.m_MatchID);
		from_chars_throw(result[3], newLobby.m_LobbyNumber);

		Lobby currentLobby;
		currentLobby.m_LobbyID = SteamID(result[4].str());
		from_chars_throw(result[5], currentLobby.m_MatchID);
		from_chars_throw(result[6], currentLobby.m_LobbyNumber);

		bool connectedToMatchServer, hasLobby, assignedMatchEnded;
		from_chars_throw(result[7], connectedToMatchServer);
		from_chars_throw(result[8], hasLobby);
		from_chars_throw(result[9], assignedMatchEnded);

		return std::make_shared<DifferingLobbyReceivedLine>(args.m_Timestamp, newLobby, currentLobby,
			connectedToMatchServer, hasLobby, assignedMatchEnded);
	}

	return nullptr;
}

void DifferingLobbyReceivedLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt(
		"Differing lobby received. Lobby: {} CurrentlyAssigned: {} ConnectedToMatchServer: {} HasLobby: {} AssignedMatchEnded: {}",
		m_NewLobby, m_CurrentLobby, int(m_ConnectedToMatchServer), int(m_HasLobby), int(m_AssignedMatchEnded));
}

namespace tf2_bot_detector
{
	template<typename CharT, typename Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
		const DifferingLobbyReceivedLine::Lobby& lobby)
	{
		return os << lobby.m_LobbyID << "/Match" << lobby.m_MatchID << "/Lobby" << lobby.m_LobbyNumber;
	}
}
