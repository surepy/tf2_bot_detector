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

#include "EdictUsageLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

EdictUsageLine::EdictUsageLine(time_point_t timestamp, uint16_t usedEdicts, uint16_t totalEdicts) :
	BaseClass(timestamp), m_UsedEdicts(usedEdicts), m_TotalEdicts(totalEdicts)
{
}

std::shared_ptr<IConsoleLine> EdictUsageLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(edicts  : (\d+) used of (\d+) max)regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		uint16_t usedEdicts, totalEdicts;
		from_chars_throw(result[1], usedEdicts);
		from_chars_throw(result[2], totalEdicts);
		return std::make_shared<EdictUsageLine>(args.m_Timestamp, usedEdicts, totalEdicts);
	}

	return nullptr;
}

void EdictUsageLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("edicts  : {} used of {} max", m_UsedEdicts, m_TotalEdicts);
}
