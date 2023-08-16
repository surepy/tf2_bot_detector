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

#include "QueueStateChangeLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

QueueStateChangeLine::QueueStateChangeLine(time_point_t timestamp, TFMatchGroup queueType,
	TFQueueStateChange stateChange) :
	BaseClass(timestamp), m_QueueType(queueType), m_StateChange(stateChange)
{
}

namespace
{
	struct QueueStateChangeType
	{
		std::string_view m_String;
		TFMatchGroup m_QueueType;
		TFQueueStateChange m_StateChange;
	};

	static constexpr QueueStateChangeType QUEUE_STATE_CHANGE_TYPES[] =
	{
		{ "[PartyClient] Requesting queue for 12v12 Casual Match", TFMatchGroup::Casual12s, TFQueueStateChange::RequestedEnter },
		{ "[PartyClient] Entering queue for match group 12v12 Casual Match", TFMatchGroup::Casual12s, TFQueueStateChange::Entered },
		{ "[PartyClient] Requesting exit queue for 12v12 Casual Match", TFMatchGroup::Casual12s, TFQueueStateChange::RequestedExit },
		{ "[PartyClient] Leaving queue for match group 12v12 Casual Match", TFMatchGroup::Casual12s, TFQueueStateChange::Exited },

		{ "[PartyClient] Requesting queue for 6v6 Ladder Match", TFMatchGroup::Competitive6s, TFQueueStateChange::RequestedEnter },
		{ "[PartyClient] Entering queue for match group 6v6 Ladder Match", TFMatchGroup::Competitive6s, TFQueueStateChange::Entered },
		{ "[PartyClient] Requesting exit queue for 6v6 Ladder Match", TFMatchGroup::Competitive6s, TFQueueStateChange::RequestedExit },
		{ "[PartyClient] Leaving queue for match group 6v6 Ladder Match", TFMatchGroup::Competitive6s, TFQueueStateChange::Exited },
	};
}

std::shared_ptr<IConsoleLine> QueueStateChangeLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	for (const auto& match : QUEUE_STATE_CHANGE_TYPES)
	{
		if (args.m_Text == match.m_String)
			return std::make_shared<QueueStateChangeLine>(args.m_Timestamp, match.m_QueueType, match.m_StateChange);
	}

	return nullptr;
}

void QueueStateChangeLine::Print(const PrintArgs& args) const
{
	for (const auto& match : QUEUE_STATE_CHANGE_TYPES)
	{
		if (match.m_QueueType == m_QueueType &&
			match.m_StateChange == m_StateChange)
		{
			ImGui::TextFmt(match.m_String);
			return;
		}
	}
}
