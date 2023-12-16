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

#include "SuicideNotificationLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

SuicideNotificationLine::SuicideNotificationLine(time_point_t timestamp, std::string name) :
	BaseClass(timestamp), m_Name(std::move(name))
{
}

SuicideNotificationLine::SuicideNotificationLine(time_point_t timestamp, std::string name, SteamID id) :
	BaseClass(timestamp), m_Name(std::move(name)), m_ID(std::move(id))
{
}

std::shared_ptr<IConsoleLine> SuicideNotificationLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex((.*) suicided.)regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		auto steamid = args.m_World.FindSteamIDForName(result[1].str());

		return std::make_shared<SuicideNotificationLine>(args.m_Timestamp, result[1].str(), steamid.has_value() ? steamid.value() : SteamID::SteamID());
	}

	return nullptr;
}

// i promise, i will refactor (3)
static std::string _killNotifMarkReasonBadFix = "";

static std::array<float, 4> chatColor = { .5f, .5f, .5f, 1.0f };

void SuicideNotificationLine::Print(const PrintArgs& args) const
{
	if (args.m_Settings.m_KillLogsInChat) {
		// all the comments and debt from KillNotificationLine applies here too.
		ImGuiDesktop::ScopeGuards::ID id(this);

		ImGui::BeginGroup();

		ImGui::TextFmt(chatColor, "-> {} // suicide", m_Name.c_str());
		ImGui::EndGroup();

		const bool isHovered = ImGui::IsItemHovered();

		if (auto scope = ImGui::BeginPopupContextItemScope("ChatConsoleLineContextMenu"))
		{
			tf2_bot_detector::DrawPlayerContextCopyMenu(m_Name.c_str(), m_ID);
			tf2_bot_detector::DrawPlayerContextGoToMenu(args.m_Settings, m_ID);

			if (m_ID.IsValid()) {
				args.m_MainWindow.DrawPlayerContextMarkMenu(m_ID, m_Name, _killNotifMarkReasonBadFix);
			}
			else {
				ImGui::TextFmt(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Marking Unavailable");
			}
		}
		else if (isHovered)
		{
			if (IPlayer* attacker = args.m_WorldState.FindPlayer(m_ID))
				args.m_MainWindow.DrawPlayerTooltip(*attacker);
		}
	}
}
