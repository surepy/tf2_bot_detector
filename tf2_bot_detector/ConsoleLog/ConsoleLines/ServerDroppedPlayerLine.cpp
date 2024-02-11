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
#include <Util/ScopeGuards.h>

#include <regex>
#include <sstream>
#include <stdexcept>

#include "ServerDroppedPlayerLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerDroppedPlayerLine::ServerDroppedPlayerLine(time_point_t timestamp, std::string playerName, std::string reason) :
	BaseClass(timestamp), m_PlayerName(std::move(playerName)), m_Reason(std::move(reason))
{
}

std::shared_ptr<IConsoleLine> ServerDroppedPlayerLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(Dropped (.*) from server \((.*)\))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		return std::make_shared<ServerDroppedPlayerLine>(args.m_Timestamp, result[1].str(), result[2].str());
	}

	return nullptr;
}

void ServerDroppedPlayerLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("Dropped {} from server ({})", m_PlayerName, m_Reason);
}
