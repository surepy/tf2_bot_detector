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

#include "ServerStatusPlayerIPLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerStatusPlayerIPLine::ServerStatusPlayerIPLine(time_point_t timestamp, std::string localIP, std::string publicIP) :
	BaseClass(timestamp), m_LocalIP(std::move(localIP)), m_PublicIP(std::move(publicIP))
{
}

std::shared_ptr<IConsoleLine> ServerStatusPlayerIPLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(udp\/ip  : (.*)  \(public ip: (.*)\))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
		return std::make_shared<ServerStatusPlayerIPLine>(args.m_Timestamp, result[1].str(), result[2].str());

	return nullptr;
}

void ServerStatusPlayerIPLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("udp/ip  : {}  (public ip: {})", m_LocalIP, m_PublicIP);
}
