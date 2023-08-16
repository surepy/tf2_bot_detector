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

#include "ServerStatusPlayerLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

ServerStatusPlayerLine::ServerStatusPlayerLine(time_point_t timestamp, PlayerStatus playerStatus) :
	BaseClass(timestamp), m_PlayerStatus(std::move(playerStatus))
{
}

std::shared_ptr<IConsoleLine> ServerStatusPlayerLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(#\s+(\d+)\s+"((?:.|[\r\n])+)"\s+(\[.*\])\s+(?:(\d+):)?(\d+):(\d+)\s+(\d+)\s+(\d+)\s+(\w+)(?:\s+(\S+))?)regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		PlayerStatus status{};

		from_chars_throw(result[1], status.m_UserID);
		status.m_Name = result[2].str();
		status.m_SteamID = SteamID(std::string_view(&*result[3].first, result[3].length()));

		// Connected time
		{
			uint32_t connectedHours = 0;
			uint32_t connectedMins;
			uint32_t connectedSecs;

			if (result[4].matched)
				from_chars_throw(result[4], connectedHours);

			from_chars_throw(result[5], connectedMins);
			from_chars_throw(result[6], connectedSecs);

			status.m_ConnectionTime = args.m_Timestamp - ((connectedHours * 1h) + (connectedMins * 1min) + connectedSecs * 1s);
		}

		from_chars_throw(result[7], status.m_Ping);
		from_chars_throw(result[8], status.m_Loss);

		// State
		{
			const auto state = std::string_view(&*result[9].first, result[9].length());
			if (state == "active"sv)
				status.m_State = PlayerStatusState::Active;
			else if (state == "spawning"sv)
				status.m_State = PlayerStatusState::Spawning;
			else if (state == "connecting"sv)
				status.m_State = PlayerStatusState::Connecting;
			else if (state == "challenging"sv)
				status.m_State = PlayerStatusState::Challenging;
			else
				throw std::runtime_error("Unknown player status state "s << std::quoted(state));
		}

		status.m_Address = result[10].str();

		return std::make_shared<ServerStatusPlayerLine>(args.m_Timestamp, std::move(status));
	}

	return nullptr;
}

void ServerStatusPlayerLine::Print(const PrintArgs& args) const
{
	const PlayerStatus& s = m_PlayerStatus;
	ImGui::Text("# %6u \"%-19s\" %-19s %4u %4u",
		s.m_UserID,
		s.m_Name.c_str(),
		s.m_SteamID.str().c_str(),
		s.m_Ping,
		s.m_Loss);
}
