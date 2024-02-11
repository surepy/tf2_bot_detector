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

#include "TeamsSwitchedLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

std::shared_ptr<IConsoleLine> TeamsSwitchedLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	if (args.m_Text == "Teams have been switched."sv)
		return std::make_unique<TeamsSwitchedLine>(args.m_Timestamp);

	return nullptr;
}

bool TeamsSwitchedLine::ShouldPrint() const
{
#ifdef _DEBUG
	return true;
#else
	return false;
#endif
}

void TeamsSwitchedLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt({ 0.98f, 0.73f, 0.01f, 1 }, "Teams have been switched.");
}
