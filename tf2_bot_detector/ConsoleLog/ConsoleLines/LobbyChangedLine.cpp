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

#include "LobbyChangedLine.h"

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

LobbyChangedLine::LobbyChangedLine(time_point_t timestamp, LobbyChangeType type) :
	BaseClass(timestamp), m_ChangeType(type)
{
}

std::shared_ptr<IConsoleLine> LobbyChangedLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	if (args.m_Text == "Lobby created"sv)
		return std::make_shared<LobbyChangedLine>(args.m_Timestamp, LobbyChangeType::Created);
	else if (args.m_Text == "Lobby updated"sv)
		return std::make_shared<LobbyChangedLine>(args.m_Timestamp, LobbyChangeType::Updated);
	else if (args.m_Text == "Lobby destroyed"sv)
		return std::make_shared<LobbyChangedLine>(args.m_Timestamp, LobbyChangeType::Destroyed);

	return nullptr;
}

bool LobbyChangedLine::ShouldPrint() const
{
	return GetChangeType() == LobbyChangeType::Created;
}

void LobbyChangedLine::Print(const PrintArgs& args) const
{
	if (ShouldPrint())
		ImGui::Separator();
}
