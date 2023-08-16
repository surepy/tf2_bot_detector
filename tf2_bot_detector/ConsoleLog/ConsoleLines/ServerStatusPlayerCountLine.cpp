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

#include "ServerStatusPlayerCountLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerStatusPlayerCountLine::ServerStatusPlayerCountLine(time_point_t timestamp, uint8_t playerCount,
	uint8_t botCount, uint8_t maxplayers) :
	BaseClass(timestamp), m_PlayerCount(playerCount), m_BotCount(botCount), m_MaxPlayers(maxplayers)
{
}

std::shared_ptr<IConsoleLine> ServerStatusPlayerCountLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(players : (\d+) humans, (\d+) bots \((\d+) max\))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		uint8_t playerCount, botCount, maxPlayers;
		from_chars_throw(result[1], playerCount);
		from_chars_throw(result[2], botCount);
		from_chars_throw(result[3], maxPlayers);
		return std::make_shared<ServerStatusPlayerCountLine>(args.m_Timestamp, playerCount, botCount, maxPlayers);
	}

	return nullptr;
}

void ServerStatusPlayerCountLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("players : {} humans, {} bots ({} max)", m_PlayerCount, m_BotCount, m_MaxPlayers);
}
