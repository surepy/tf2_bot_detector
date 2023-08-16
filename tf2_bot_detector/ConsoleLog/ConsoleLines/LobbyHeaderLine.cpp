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

#include "LobbyHeaderLine.h"

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

LobbyHeaderLine::LobbyHeaderLine(time_point_t timestamp, unsigned memberCount, unsigned pendingCount) :
	BaseClass(timestamp), m_MemberCount(memberCount), m_PendingCount(pendingCount)
{
}

std::shared_ptr<IConsoleLine> LobbyHeaderLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(CTFLobbyShared: ID:([0-9a-f]*)\s+(\d+) member\(s\), (\d+) pending)regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		unsigned memberCount, pendingCount;
		if (!mh::from_chars(std::string_view(&*result[2].first, result[2].length()), memberCount))
			throw std::runtime_error("Failed to parse lobby member count");
		if (!mh::from_chars(std::string_view(&*result[3].first, result[3].length()), pendingCount))
			throw std::runtime_error("Failed to parse lobby pending member count");

		return std::make_shared<LobbyHeaderLine>(args.m_Timestamp, memberCount, pendingCount);
	}

	return nullptr;
}

void LobbyHeaderLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt({ 1.0f, 1.0f, 0.8f, 1.0f }, "CTFLobbyShared: {} member(s), {} pending",
		m_MemberCount, m_PendingCount);
}
