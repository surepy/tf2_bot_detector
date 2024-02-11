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

#include "KillNotificationLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

KillNotificationLine::KillNotificationLine(time_point_t timestamp, std::string attackerName,
	std::string victimName, std::string weaponName, bool wasCrit) :
	BaseClass(timestamp), m_AttackerName(std::move(attackerName)), m_VictimName(std::move(victimName)),
	m_WeaponName(std::move(weaponName)), m_WasCrit(wasCrit)
{
}

KillNotificationLine::KillNotificationLine(time_point_t timestamp, std::string attackerName, SteamID attacker,
	std::string victimName, SteamID victim, std::string weaponName, bool wasCrit) :
	BaseClass(timestamp), m_AttackerName(std::move(attackerName)), m_VictimName(std::move(victimName)),
	m_WeaponName(std::move(weaponName)), m_WasCrit(wasCrit), m_Attacker(std::move(attacker)), m_Victim(std::move(victim))
{
}

std::shared_ptr<IConsoleLine> KillNotificationLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex((.*) killed (.*) with (.*)\.( \(crit\))?)regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		auto attacker = args.m_World.FindSteamIDForName(result[1].str());
		auto victim = args.m_World.FindSteamIDForName(result[2].str());

		return std::make_shared<KillNotificationLine>(args.m_Timestamp,
			result[1].str(), attacker.has_value() ? attacker.value() : SteamID::SteamID(),
			result[2].str(), victim.has_value() ? victim.value() : SteamID::SteamID(),
			result[3].str(), result[4].matched
		);
	}

	return nullptr;
}

// i promise, i will refactor
static std::string _killNotifMarkReasonBadFix = "";

void KillNotificationLine::Print(const PrintArgs& args) const
{
	if (args.m_Settings.m_KillLogsInChat) {
		/*
		auto& colorSettings = args.m_Settings.m_Theme.m_Colors;
		std::array<float, 4> colors{ 0.8f, 0.8f, 1.0f, 1.0f };

		if (m_AttackerName.)
			colors = colorSettings.m_ChatLogYouFG;
		else if (msgLine.GetTeamShareResult() == TeamShareResult::SameTeams)
			colors = colorSettings.m_ChatLogFriendlyTeamFG;
		else if (msgLine.GetTeamShareResult() == TeamShareResult::OppositeTeams)
			colors = colorSettings.m_ChatLogEnemyTeamFG;
		*/

		// TODO: stare at how chatconsoleline::print or ProcessChatMessage works
		// TODO: COPYPASTED CODE FROM ChatConsoleLine::Print; move into a seperate function!
		ImGuiDesktop::ScopeGuards::ID id(this);

		ImGui::BeginGroup();

		auto& colorSettings = args.m_Settings.m_Theme.m_Colors;
		std::array<float, 4> chatColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		TeamShareResult team = args.m_WorldState.GetTeamShareResult(m_Attacker);

		if (m_Attacker == args.m_Settings.GetLocalSteamID()) {
			chatColor = colorSettings.m_ChatLogYouFG;
		}
		else if (team == TeamShareResult::OppositeTeams) {
			chatColor = colorSettings.m_ChatLogEnemyTeamFG;
		}
		else if (team == TeamShareResult::SameTeams) {
			chatColor = colorSettings.m_ChatLogFriendlyTeamFG;
		}

		// TODO: [POTENTIAL PERFORMANCE ISSUE/TECH DEBT] this is kind of a waste doing this same calculation over and over again, and might get out of hand pretty quickly.
		// move into a seperate pre-calculated variable
		// what this does is it just makes the color darker than the actual chat..
		chatColor[0] = chatColor[0] / 2;
		chatColor[1] = chatColor[1] / 2;
		chatColor[2] = chatColor[2] / 2;

		ImGui::TextFmt(chatColor, "{} -> {} // {} {}", m_AttackerName.c_str(),
			m_VictimName.c_str(), m_WeaponName.c_str(), m_WasCrit ? "(crit)" : "");
		ImGui::EndGroup();

		const bool isHovered = ImGui::IsItemHovered();

		if (auto scope = ImGui::BeginPopupContextItemScope("ChatConsoleLineContextMenu"))
		{
			tf2_bot_detector::DrawPlayerContextCopyMenu(m_AttackerName.c_str(), m_Attacker);
			tf2_bot_detector::DrawPlayerContextGoToMenu(args.m_Settings, m_Attacker);

			if (m_Attacker.IsValid()) {
				args.m_MainWindow.DrawPlayerContextMarkMenu(m_Attacker, m_AttackerName, _killNotifMarkReasonBadFix);
			}
			else {
				ImGui::TextFmt(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Marking Unavailable");
			}

			// ImGui::TextFmt(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), m_Attacker.str());
		}
		else if (isHovered)
		{
			if (IPlayer* attacker = args.m_WorldState.FindPlayer(m_Attacker))
				args.m_MainWindow.DrawPlayerTooltip(*attacker);
		}
	}
}
