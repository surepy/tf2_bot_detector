#pragma once

#include "Clock.h"
#include "GameData/TFParty.h"
#include "LobbyMember.h"
#include "PlayerStatus.h"
#include "ConsoleLog/IConsoleLine.h"
#include "GameData/IPlayer.h"

#include <mh/reflection/enum.hpp>

#include <array>
#include <memory>
#include <string_view>

namespace tf2_bot_detector
{
	enum class TeamShareResult;
	enum class TFClassType;
	enum class TFMatchGroup;
	enum class TFQueueStateChange;
	enum class UserMessageType;

	/// <summary>
	/// SVCUserMessage debug line types.
	/// </summary>
	class SVCUserMessageLine final : public ConsoleLineBase<SVCUserMessageLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		SVCUserMessageLine(time_point_t timestamp, std::string address, UserMessageType type, uint16_t bytes);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::SVC_UserMessage; }
		bool ShouldPrint() const override;
		void Print(const PrintArgs& args) const override;

		const std::string& GetAddress() const { return m_Address; }
		UserMessageType GetUserMessageType() const { return m_MsgType; }
		uint16_t GetUserMessageBytes() const { return m_MsgBytes; }

	private:
		static bool IsSpecial(UserMessageType type);

		std::string m_Address{};
		UserMessageType m_MsgType{};
		uint16_t m_MsgBytes{};
	};
}
