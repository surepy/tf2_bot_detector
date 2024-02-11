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

#include "InQueueLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

InQueueLine::InQueueLine(time_point_t timestamp, TFMatchGroup queueType, time_point_t queueStartTime) :
	BaseClass(timestamp), m_QueueType(queueType), m_QueueStartTime(queueStartTime)
{
}

std::shared_ptr<IConsoleLine> InQueueLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(
		R"regex(    MatchGroup: (\d+)\s+Started matchmaking:\s+(.*)\s+\(\d+ seconds ago, now is (.*)\))regex",
		std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		TFMatchGroup matchGroup = TFMatchGroup::Invalid;
		{
			uint8_t matchGroupRaw{};
			from_chars_throw(result[1], matchGroupRaw);
			matchGroup = TFMatchGroup(matchGroupRaw);
		}

		time_point_t startTime{};
		{
			std::tm startTimeFull{};
			std::istringstream ss;
			ss.str(result[2].str());
			//ss >> std::get_time(&startTimeFull, "%c");
			ss >> std::get_time(&startTimeFull, "%a %b %d %H:%M:%S %Y");

			startTimeFull.tm_isdst = -1; // auto-detect DST
			startTime = clock_t::from_time_t(std::mktime(&startTimeFull));
			if (startTime.time_since_epoch().count() < 0)
			{
				LogError(MH_SOURCE_LOCATION_CURRENT(), "Failed to parse "s << std::quoted(result[2].str()) << " as a timestamp");
				return nullptr;
			}
		}

		return std::make_shared<InQueueLine>(args.m_Timestamp, matchGroup, startTime);
	}

	return nullptr;
}

void InQueueLine::Print(const PrintArgs& args) const
{
	char timeBufNow[128];
	const auto localTM = GetLocalTM();
	strftime(timeBufNow, sizeof(timeBufNow), "%c", &localTM);

	char timeBufThen[128];
	const auto queueStartTimeTM = ToTM(m_QueueStartTime);
	strftime(timeBufThen, sizeof(timeBufThen), "%c", &queueStartTimeTM);

	const uint64_t seconds = to_seconds<uint64_t>(clock_t::now() - m_QueueStartTime);

	ImGui::TextFmt("    MatchGroup: {}  Started matchmaking: {} ({} seconds ago, now is {})",
		uint32_t(m_QueueType), timeBufThen, seconds, timeBufNow);
}
