#include "IConsoleLine.h"
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

#undef GetMessage

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;


IConsoleLine::IConsoleLine(time_point_t timestamp) :
	m_Timestamp(timestamp)
{
}

auto IConsoleLine::GetTypeData() -> std::list<ConsoleLineTypeData>&
{
	static std::list<ConsoleLineTypeData> s_List;
	return s_List;
}

std::shared_ptr<IConsoleLine> IConsoleLine::ParseConsoleLine(const std::string_view& text, time_point_t timestamp, IWorldState& world)
{
	auto& list = GetTypeData();

	if ((s_TotalParseCount % 1024) == 0)
	{
		// Periodically re-sort the line types for best performance
		list.sort([](ConsoleLineTypeData& lhs, ConsoleLineTypeData& rhs)
			{
				if (rhs.m_AutoParse != lhs.m_AutoParse)
					return rhs.m_AutoParse < lhs.m_AutoParse;

				// Intentionally reversed, we want descending order
				return rhs.m_AutoParseSuccessCount < lhs.m_AutoParseSuccessCount;
			});
	}

	s_TotalParseCount++;

	const ConsoleLineTryParseArgs args{ text, timestamp, world };
	for (auto& data : list)
	{
		if (!data.m_AutoParse)
			continue;

		auto parsed = data.m_TryParseFunc(args);
		if (!parsed)
			continue;

		data.m_AutoParseSuccessCount++;
		return parsed;
	}

	//if (auto chatLine = ChatConsoleLine::TryParse(text, timestamp))
	//	return chatLine;

	return nullptr;
	//return std::make_shared<GenericConsoleLine>(timestamp, std::string(text));
}

void IConsoleLine::AddTypeData(ConsoleLineTypeData data)
{
	GetTypeData().push_back(std::move(data));
}

#ifdef TF2BD_ENABLE_TESTS
#include <catch2/catch.hpp>

TEST_CASE("ProcessChatMessage - Newlines", "[tf2bd]")
{
	const auto TestProcessChatMessage = [](std::string message)
	{
		ChatConsoleLine line(tfbd_clock_t::now(), "<playername>", std::move(message), false, false, false, TeamShareResult::SameTeams, SteamID{});

		Settings::Theme dummyTheme;

		std::string output;

		ProcessChatMessage(line, dummyTheme,
			[&](const ImVec4& color, const std::string_view& msg)
			{
				output << msg << '\n';
			},
			[&]()
			{
				output << "<sameline>";
			});

		return output;
	};

	REQUIRE(TestProcessChatMessage("this is a normal message") ==
		"<playername>\n<sameline>: \n<sameline>this is a normal message\n");
	REQUIRE(TestProcessChatMessage("this is\n\na message with newlines in it") ==
		"<playername>\n<sameline>: \n<sameline>this is\n<sameline>(\\n x 2)\n<playername>\n<sameline>: \n<sameline>a message with newlines in it\n");
}

#endif
