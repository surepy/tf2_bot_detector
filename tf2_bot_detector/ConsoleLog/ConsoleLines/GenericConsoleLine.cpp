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

#include "GenericConsoleLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

GenericConsoleLine::GenericConsoleLine(time_point_t timestamp, std::string text) :
	BaseClass(timestamp), m_Text(std::move(text))
{
	m_Text.shrink_to_fit();
}

std::shared_ptr<IConsoleLine> GenericConsoleLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	return std::make_shared<GenericConsoleLine>(args.m_Timestamp, std::string(args.m_Text));
}

void GenericConsoleLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt(m_Text);
}
