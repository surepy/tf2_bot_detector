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

#include "ConnectingLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

ConnectingLine::ConnectingLine(time_point_t timestamp, std::string address, bool isMatchmaking, bool isRetrying) :
	BaseClass(timestamp), m_Address(std::move(address)), m_IsMatchmaking(isMatchmaking), m_IsRetrying(isRetrying)
{
}

std::shared_ptr<IConsoleLine> ConnectingLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	{
		static const std::regex s_ConnectingRegex(R"regex(Connecting to( matchmaking server)? (.*?)(\.\.\.)?)regex", std::regex::optimize);
		if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_ConnectingRegex))
			return std::make_shared<ConnectingLine>(args.m_Timestamp, result[2].str(), result[1].matched, false);
	}

	{
		static const std::regex s_RetryingRegex(R"regex(Retrying (.*)\.\.\.)regex", std::regex::optimize);
		if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_RetryingRegex))
			return std::make_shared<ConnectingLine>(args.m_Timestamp, result[1].str(), false, true);
	}

	return nullptr;
}

void ConnectingLine::Print(const PrintArgs& args) const
{
	if (m_IsMatchmaking)
		ImGui::TextFmt("Connecting to matchmaking server {}...", m_Address);
	else if (m_IsRetrying)
		ImGui::TextFmt("Retrying {}...", m_Address);
	else
		ImGui::TextFmt("Connecting to {}...", m_Address);
}
