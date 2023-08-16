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

#include "PartyHeaderLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

PartyHeaderLine::PartyHeaderLine(time_point_t timestamp, TFParty party) :
	BaseClass(timestamp), m_Party(std::move(party))
{
}

std::shared_ptr<IConsoleLine> PartyHeaderLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(TFParty:\s+ID:([0-9a-f]+)\s+(\d+) member\(s\)\s+LeaderID: (\[.*\]))regex", std::regex::optimize);
	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		TFParty party{};

		{
			uint64_t partyID;
			from_chars_throw(result[1], partyID, 16);
			party.m_PartyID = TFPartyID(partyID);
		}

		from_chars_throw(result[2], party.m_MemberCount);

		party.m_LeaderID = SteamID(result[3].str());

		return std::make_shared<PartyHeaderLine>(args.m_Timestamp, std::move(party));
	}

	return nullptr;
}

void PartyHeaderLine::Print(const PrintArgs& args) const
{
	ImGui::Text("TFParty: ID:%xll  %u member(s)  LeaderID: %s", m_Party.m_PartyID,
		m_Party.m_MemberCount, m_Party.m_LeaderID.str().c_str());
}
