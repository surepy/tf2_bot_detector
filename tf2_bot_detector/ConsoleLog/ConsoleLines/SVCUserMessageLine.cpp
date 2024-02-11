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

#include "SVCUserMessageLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

SVCUserMessageLine::SVCUserMessageLine(time_point_t timestamp, std::string address, UserMessageType type, uint16_t bytes) :
	BaseClass(timestamp), m_Address(std::move(address)), m_MsgType(type), m_MsgBytes(bytes)
{
}

std::shared_ptr<IConsoleLine> SVCUserMessageLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex(Msg from ((?:\d+\.\d+\.\d+\.\d+:\d+)|loopback): svc_UserMessage: type (\d+), bytes (\d+))regex", std::regex::optimize);

	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		uint16_t type, bytes;
		from_chars_throw(result[2], type);

		from_chars_throw(result[3], bytes);

		return std::make_shared<SVCUserMessageLine>(args.m_Timestamp, result[1].str(), UserMessageType(type), bytes);
	}

	return nullptr;
}

bool SVCUserMessageLine::IsSpecial(UserMessageType type)
{
#ifdef _DEBUG
	return mh::any_eq(type,
		UserMessageType::CallVoteFailed,
		UserMessageType::VoteFailed,
		UserMessageType::VotePass,
		UserMessageType::VoteSetup,
		UserMessageType::VoteStart);
#else
	return false;
#endif
}

bool SVCUserMessageLine::ShouldPrint() const
{
	return IsSpecial(m_MsgType);
}

void SVCUserMessageLine::Print(const PrintArgs& args) const
{
	if (IsSpecial(m_MsgType))
		ImGui::TextFmt({ 0, 1, 1, 1 }, "{}", mh::enum_fmt(m_MsgType));
	else
		ImGui::TextFmt("Msg from {}: svc_UserMessage: type {}, bytes {}", m_Address, int(m_MsgType), m_MsgBytes);
}
