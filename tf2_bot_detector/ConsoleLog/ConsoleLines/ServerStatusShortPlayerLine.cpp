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

#include "ServerStatusShortPlayerLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerStatusShortPlayerLine::ServerStatusShortPlayerLine(time_point_t timestamp, PlayerStatusShort playerStatus) :
	BaseClass(timestamp), m_PlayerStatus(std::move(playerStatus))
{
}

std::shared_ptr<IConsoleLine> ServerStatusShortPlayerLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(#(\d+) - (.+))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		PlayerStatusShort status{};

		from_chars_throw(result[1], status.m_ClientIndex);
		assert(status.m_ClientIndex >= 1);
		status.m_Name = result[2].str();

		return std::make_shared<ServerStatusShortPlayerLine>(args.m_Timestamp, std::move(status));
	}

	return nullptr;
}

void ServerStatusShortPlayerLine::Print(const PrintArgs& args) const
{
	ImGui::Text("#%u - %s", m_PlayerStatus.m_ClientIndex, m_PlayerStatus.m_Name.c_str());
}
