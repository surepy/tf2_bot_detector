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

	class ConnectingLine final : public ConsoleLineBase<ConnectingLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ConnectingLine(time_point_t timestamp, std::string address, bool isMatchmaking, bool isRetrying);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::Connecting; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		const std::string& GetAddress() const { return m_Address; }

	private:
		std::string m_Address;
		bool m_IsMatchmaking : 1;
		bool m_IsRetrying : 1;
	};
}
