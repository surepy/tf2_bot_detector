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

#include "ChatConsoleLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

ChatConsoleLine::ChatConsoleLine(time_point_t timestamp, std::string playerName, std::string message,
	bool isDead, bool isTeam, bool isSelf, TeamShareResult teamShareResult, SteamID id) :
	ConsoleLineBase(timestamp), m_PlayerName(std::move(playerName)), m_Message(std::move(message)),
	m_IsDead(isDead), m_IsTeam(isTeam), m_IsSelf(isSelf), m_TeamShareResult(teamShareResult), m_PlayerSteamID(id)
{
	m_PlayerName.shrink_to_fit();
	m_Message.shrink_to_fit();
}

// this is a bad fix, but we can't really access PlayerExtraData (+ the fact that they will be destroyed when the player leave will screw over a lot of stuff)
std::string ChatConsoleLine::m_PendingMarkReason = "";

std::shared_ptr<IConsoleLine> ChatConsoleLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	LogError(MH_SOURCE_LOCATION_CURRENT(), "This should never happen!");
	return nullptr;
}

#if 0
std::shared_ptr<ChatConsoleLine> ChatConsoleLine::TryParseFlexible(const std::string_view& text, time_point_t timestamp)
{
	return TryParse(text, timestamp, true);
}

std::shared_ptr<ChatConsoleLine> ChatConsoleLine::TryParse(const std::string_view& text, time_point_t timestamp, bool flexible)
{
	if (!flexible)
		LogWarning("Found ourselves searching chat message "s << std::quoted(text) << " in non-flexible mode");

	static const std::regex s_Regex(R"regex((\*DEAD\*)?\s*(\(TEAM\))?\s*(.{1,33}) :  ((?:.|[\r\n])*))regex", std::regex::optimize);
	static const std::regex s_RegexFlexible(R"regex((\*DEAD\*)?\s*(\(TEAM\))?\s*((?:.|\s){1,33}?) :  ((?:.|[\r\n])*))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(text.begin(), text.end(), result, flexible ? s_RegexFlexible : s_Regex))
	{
		return std::make_shared<ChatConsoleLine>(timestamp, result[3].str(), result[4].str(),
			result[1].matched, result[2].matched);
	}

	return nullptr;
}
#endif

template<typename TTextFunc, typename TSameLineFunc>
static void ProcessChatMessage(const ChatConsoleLine& msgLine, const Settings::Theme& theme,
	TTextFunc&& textFunc, TSameLineFunc&& sameLineFunc)
{
	auto& colorSettings = theme.m_Colors;
	std::array<float, 4> colors{ 0.8f, 0.8f, 1.0f, 1.0f };

	if (msgLine.IsSelf())
		colors = colorSettings.m_ChatLogYouFG;
	else if (msgLine.GetTeamShareResult() == TeamShareResult::SameTeams)
		colors = colorSettings.m_ChatLogFriendlyTeamFG;
	else if (msgLine.GetTeamShareResult() == TeamShareResult::OppositeTeams)
		colors = colorSettings.m_ChatLogEnemyTeamFG;

	const auto PrintLHS = [&](float alphaScale = 1.0f)
	{
		if (msgLine.IsDead())
		{
			textFunc(ImVec4(0.5f, 0.5f, 0.5f, 1.0f * alphaScale), "*DEAD*");
			sameLineFunc();
		}

		if (msgLine.IsTeam())
		{
			textFunc(ImVec4(colors[0], colors[1], colors[2], colors[3] * 0.75f * alphaScale), "(TEAM)");
			sameLineFunc();
		}

		textFunc(ImVec4(colors[0], colors[1], colors[2], colors[3] * alphaScale), msgLine.GetPlayerName());
		sameLineFunc();

		textFunc(ImVec4(1, 1, 1, alphaScale), ": ");
		sameLineFunc();
	};

	PrintLHS();

	const auto& msg = msgLine.GetMessage();
	const ImVec4 msgColor(0.8f, 0.8f, 0.8f, 1.0f);
	if (msg.find('\n') == msg.npos)
	{
		textFunc(msgColor, msg);
	}
	else
	{
		// collapse groups of newlines in the message into red "(\n x <count>)" text
		bool firstLine = true;
		for (size_t i = 0; i < msg.size(); )
		{
			if (!firstLine)
			{
				PrintLHS(0.5f);
			}

			size_t nonNewlineEnd = std::min(msg.find('\n', i), msg.size());
			{
				auto start = msg.data() + i;
				auto end = msg.data() + nonNewlineEnd;
				textFunc({ 1, 1, 1, 1 }, std::string_view(start, end - start));
			}

			size_t newlineEnd = std::min(msg.find_first_not_of('\n', nonNewlineEnd), msg.size());

			if (newlineEnd > nonNewlineEnd)
			{
				sameLineFunc();
				textFunc(ImVec4(1, 0.5f, 0.5f, 1.0f),
					mh::fmtstr<64>("(\\n x {})", (newlineEnd - nonNewlineEnd)).c_str());
			}

			i = newlineEnd;
			firstLine = false;
		}
	}
}

void ChatConsoleLine::Print(const PrintArgs& args) const
{
	ImGuiDesktop::ScopeGuards::ID id(this);

	ImGui::BeginGroup();
	ProcessChatMessage(
		*this,
		args.m_Settings.m_Theme,
		[](const ImVec4& color, const std::string_view& msg) {
			// TODO: selectable text?
			//ImGuiDesktop::TextColor scope(color);
			//auto message = mh::fmtstr<3073>(msg);
			//std::string str = message.str();
			//ImGui::InputText("", &str, ImGuiInputTextFlags_ReadOnly);

			ImGui::TextFmt(color, msg);
		},
		[] { ImGui::SameLine(); }
		);
	ImGui::EndGroup();

	const bool isHovered = ImGui::IsItemHovered();

	if (auto scope = ImGui::BeginPopupContextItemScope("ChatConsoleLineContextMenu"))
	{
		if (ImGui::MenuItem("Copy Text"))
		{
			std::string fullText;
			ImGui::Selectable("test");

			ProcessChatMessage(*this, args.m_Settings.m_Theme,
				[&](const ImVec4&, const std::string_view& msg)
				{
					if (!fullText.empty())
						fullText += ' ';

					fullText.append(msg);
				},
				[] {});

			ImGui::SetClipboardText(fullText.c_str());
		}

		tf2_bot_detector::DrawPlayerContextCopyMenu(m_PlayerName.c_str(), m_PlayerSteamID);
		tf2_bot_detector::DrawPlayerContextGoToMenu(args.m_Settings, m_PlayerSteamID);

		if (m_PlayerSteamID.IsValid()) {
			args.m_MainWindow.DrawPlayerContextMarkMenu(m_PlayerSteamID, m_PlayerName, m_PendingMarkReason);
		}
		else {
			ImGui::TextFmt(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Marking Unavailable");
		}

		// ImGui::TextFmt(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), m_PlayerSteamID.str());
	}
	else if (isHovered)
	{
		if (auto player = args.m_WorldState.FindPlayer(m_PlayerSteamID))
			args.m_MainWindow.DrawPlayerTooltip(*player);
	}
}
