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

#include "HostNewGameLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

std::shared_ptr<IConsoleLine> HostNewGameLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	if (args.m_Text == "---- Host_NewGame ----"sv)
		return std::make_shared<HostNewGameLine>(args.m_Timestamp);

	return nullptr;
}

void HostNewGameLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("---- Host_NewGame ----"sv);
}
