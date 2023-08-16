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

#include "LobbyMemberLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

LobbyMemberLine::LobbyMemberLine(time_point_t timestamp, const LobbyMember& lobbyMember) :
	BaseClass(timestamp), m_LobbyMember(lobbyMember)
{
}

std::shared_ptr<IConsoleLine> LobbyMemberLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(\s+(?:(?:Member)|(Pending))\[(\d+)\] (\[.*\])\s+team = (\w+)\s+type = (\w+))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		LobbyMember member{};
		member.m_Pending = result[1].matched;

		if (!mh::from_chars(std::string_view(&*result[2].first, result[2].length()), member.m_Index))
			throw std::runtime_error("Failed to parse lobby member regex");

		member.m_SteamID = SteamID(std::string_view(&*result[3].first, result[3].length()));

		std::string_view teamStr(&*result[4].first, result[4].length());

		if (teamStr == "TF_GC_TEAM_DEFENDERS"sv)
			member.m_Team = LobbyMemberTeam::Defenders;
		else if (teamStr == "TF_GC_TEAM_INVADERS"sv)
			member.m_Team = LobbyMemberTeam::Invaders;
		else
			throw std::runtime_error("Unknown lobby member team");

		std::string_view typeStr(&*result[5].first, result[5].length());
		if (typeStr == "MATCH_PLAYER"sv)
			member.m_Type = LobbyMemberType::Player;
		else if (typeStr == "INVALID_PLAYER"sv)
			member.m_Type = LobbyMemberType::InvalidPlayer;
		else
			throw std::runtime_error("Unknown lobby member type");

		return std::make_shared<LobbyMemberLine>(args.m_Timestamp, member);
	}

	return nullptr;
}

void LobbyMemberLine::Print(const PrintArgs& args) const
{
	const char* team;
	switch (m_LobbyMember.m_Team)
	{
	case LobbyMemberTeam::Invaders: team = "TF_GC_TEAM_ATTACKERS"; break;
	case LobbyMemberTeam::Defenders: team = "TF_GC_TEAM_INVADERS"; break;
	default: throw std::runtime_error("Unexpected lobby member team");
	}

	assert(m_LobbyMember.m_Type == LobbyMemberType::Player);
	const char* type = "MATCH_PLAYER";

	ImGui::TextFmt({ 0.8f, 0.0f, 0.8f, 1 }, "  Member[{}] {}  team = {}  type = {}",
		m_LobbyMember.m_Index, m_LobbyMember.m_SteamID, team, type);
}
