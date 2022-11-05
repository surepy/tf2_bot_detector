#include "Actions.h"

#include <mh/text/string_insertion.hpp>
#include <mh/text/stringops.hpp>
#include <nlohmann/json.hpp>

#include <iomanip>
#include <ostream>

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

KickAction::KickAction(uint16_t userID, KickReason reason) :
	GenericCommandAction("callvote", MakeArgs(userID, reason))
{
}

duration_t KickAction::GetMinInterval() const
{
	return 5s;
}

std::string KickAction::MakeArgs(uint16_t userID, KickReason reason)
{
	const char* reasonString = [&]()
	{
		switch (reason)
		{
		case KickReason::Other:     return "other";
		case KickReason::Cheating:  return "cheating";
		case KickReason::Idle:      return "idle";
		case KickReason::Scamming:  return "scamming";
		default: throw std::runtime_error("Unknown/invalid reason");
		}
	}();

	return "kick \""s << userID << ' ' << reasonString << '"';
}

GenericCommandAction::GenericCommandAction(std::string cmd, std::string args) :
	m_Command(std::move(cmd)), m_Args(std::move(args))
{
}

void GenericCommandAction::WriteCommands(ICommandWriter& writer) const
{
	writer.Write(m_Command, m_Args);
}

ChatMessageAction::ChatMessageAction(const std::string_view& message, ChatMessageType type) :
	GenericCommandAction(std::string(GetCommand(type)), ScrubMessage(std::string(message)))
{
}

duration_t ChatMessageAction::GetMinInterval() const
{
	return 1s; // Actual cooldown is 0.66 seconds
}

std::string_view ChatMessageAction::GetCommand(ChatMessageType type)
{
	switch (type)
	{
	case ChatMessageType::Public:  return "say"sv;
	case ChatMessageType::Team:    return "say_team"sv;
	case ChatMessageType::Party:   return "say_party"sv;
	case ChatMessageType::PartyChat:   return "tf_party_chat"sv;

	default:
		throw std::invalid_argument("Unhandled/invalid ChatMessageType");
	}
}

std::string ChatMessageAction::ScrubMessage(std::string msg)
{
	msg.erase(std::remove_if(msg.begin(), msg.end(),
		[](char c)
		{
			return
				c == '\r' ||
				c == '\0' ||
				c == '\n';
		}), msg.end());

	for (auto& c : msg)
	{
		if (c == '"')
			c = '\'';
	}

	return "\""s << msg << '"';
}

void LobbyUpdateAction::WriteCommands(ICommandWriter& writer) const
{
	writer.Write("tf_lobby_debug");
}

void tf2_bot_detector::to_json(nlohmann::json& j, const KickReason& d)
{
	switch (d)
	{
	case KickReason::Other:     j = "other";     break;
	case KickReason::Cheating:  j = "cheating";  break;
	case KickReason::Idle:      j = "idle";      break;
	case KickReason::Scamming:  j = "scamming";  break;
	}
}

void tf2_bot_detector::from_json(const nlohmann::json& j, KickReason& d)
{
	const auto value = j.get<std::string_view>();

	if (value == "other")
		d = KickReason::Other;
	else if (value == "cheating")
		d = KickReason::Cheating;
	else if (value == "idle")
		d = KickReason::Idle;
	else if (value == "scamming")
		d = KickReason::Scamming;
	else
		throw std::invalid_argument(mh::format("{}: unexpected value {}", MH_SOURCE_LOCATION_CURRENT(), std::quoted(value)));
}
