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

#include "PingLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

PingLine::PingLine(time_point_t timestamp, uint16_t ping, std::string playerName) :
	BaseClass(timestamp), m_Ping(ping), m_PlayerName(std::move(playerName))
{
}

std::shared_ptr<IConsoleLine> PingLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex( *(\d+) ms : (.{1,32}))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		uint16_t ping;
		from_chars_throw(result[1], ping);
		return std::make_shared<PingLine>(args.m_Timestamp, ping, result[2].str());
	}

	return nullptr;
}

void PingLine::Print(const PrintArgs& args) const
{
	ImGui::Text("%4u : %s", m_Ping, m_PlayerName.c_str());
}
