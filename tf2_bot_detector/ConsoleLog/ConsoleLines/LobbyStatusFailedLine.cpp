#include "Application.h"
#include "Config/Settings.h"
#include "GameData/MatchmakingQueue.h"
#include "GameData/UserMessageType.h"
#include "UI/MainWindow.h"
#include "UI/ImGui_TF2BotDetector.h"
#include "Util/RegexUtils.h"
#include "Log.h"
#include "WorldState.h"

#include <imgui_desktop/ScopeGuards.h>

#include <regex>
#include <sstream>
#include <stdexcept>

#include "LobbyStatusFailedLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

std::shared_ptr<IConsoleLine> LobbyStatusFailedLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	if (args.m_Text == "Failed to find lobby shared object"sv)
		return std::make_shared<LobbyStatusFailedLine>(args.m_Timestamp);

	return nullptr;
}

void LobbyStatusFailedLine::Print(const PrintArgs& args) const
{
	ImGui::Text("Failed to find lobby shared object");
}
