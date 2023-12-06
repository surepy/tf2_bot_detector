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

#include "ConfigExecLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

ConfigExecLine::ConfigExecLine(time_point_t timestamp, std::string configFileName, bool success) :
	BaseClass(timestamp), m_ConfigFileName(std::move(configFileName)), m_Success(success)
{
}

std::shared_ptr<IConsoleLine> ConfigExecLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	// Success
	constexpr auto prefix = "execing "sv;
	if (args.m_Text.starts_with(prefix))
		return std::make_shared<ConfigExecLine>(args.m_Timestamp, std::string(args.m_Text.substr(prefix.size())), true);

	// Failure
	static const std::regex s_Regex(R"regex('(.*)' not present; not executing\.)regex", std::regex::optimize);
	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
		return std::make_shared<ConfigExecLine>(args.m_Timestamp, result[1].str(), false);

	return nullptr;
}

void ConfigExecLine::Print(const PrintArgs& args) const
{
	if (m_Success)
		ImGui::Text("execing %s", m_ConfigFileName.c_str());
	else
		ImGui::Text("'%s' not present; not executing.", m_ConfigFileName.c_str());
}
