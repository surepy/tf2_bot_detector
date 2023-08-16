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
#include <imgui_desktop/ScopeGuards.h>

#include <regex>
#include <sstream>
#include <stdexcept>

#include "ServerJoinLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerJoinLine::ServerJoinLine(time_point_t timestamp, std::string hostName, std::string mapName,
	uint8_t playerCount, uint8_t playerMaxCount, uint32_t buildNumber, uint32_t serverNumber) :
	BaseClass(timestamp), m_HostName(std::move(hostName)), m_MapName(std::move(mapName)), m_PlayerCount(playerCount),
	m_PlayerMaxCount(playerMaxCount), m_BuildNumber(buildNumber), m_ServerNumber(serverNumber)
{
}

std::shared_ptr<IConsoleLine> ServerJoinLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(
		R"regex(\n(.*)\nMap: (.*)\nPlayers: (\d+) \/ (\d+)\nBuild: (\d+)\nServer Number: (\d+)\s+)regex",
		std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		uint32_t buildNumber, serverNumber;
		from_chars_throw(result[5], buildNumber);
		from_chars_throw(result[6], serverNumber);

		uint8_t playerCount, playerMaxCount;
		from_chars_throw(result[3], playerCount);
		from_chars_throw(result[4], playerMaxCount);

		return std::make_shared<ServerJoinLine>(args.m_Timestamp, result[1].str(), result[2].str(),
			playerCount, playerMaxCount, buildNumber, serverNumber);
	}

	return nullptr;
}

void ServerJoinLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("\n{}\nMap: {}\nPlayers: {} / {}\nBuild: {}\nServer Number: {}\n",
		m_HostName, m_MapName, m_PlayerCount, m_PlayerMaxCount, m_BuildNumber, m_ServerNumber);
}
