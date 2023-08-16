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

#include "ServerStatusMapLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerStatusMapLine::ServerStatusMapLine(time_point_t timestamp, std::string mapName,
	const std::array<float, 3>& position) :
	BaseClass(timestamp), m_MapName(std::move(mapName)), m_Position(position)
{
}

std::shared_ptr<IConsoleLine> ServerStatusMapLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(map     : (.*) at: ((?:-|\d)+) x, ((?:-|\d)+) y, ((?:-|\d)+) z)regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		std::array<float, 3> pos{};
		from_chars_throw(result[2], pos[0]);
		from_chars_throw(result[3], pos[1]);
		from_chars_throw(result[4], pos[2]);

		return std::make_shared<ServerStatusMapLine>(args.m_Timestamp, result[1].str(), pos);
	}

	return nullptr;
}

void ServerStatusMapLine::Print(const PrintArgs& args) const
{
	ImGui::Text("map     : %s at: %1.0f x, %1.0f y, %1.0f z", m_MapName.c_str(),
		m_Position[0], m_Position[1], m_Position[2]);
}
