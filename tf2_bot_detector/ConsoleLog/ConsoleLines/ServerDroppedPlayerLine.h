#pragma once

#include "Clock.h"
#include "GameData/TFParty.h"
#include "LobbyMember.h"
#include "PlayerStatus.h"
#include "ConsoleLog/IConsoleLine.h"
#include "IPlayer.h"

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

	class ServerDroppedPlayerLine final : public ConsoleLineBase<ServerDroppedPlayerLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerDroppedPlayerLine(time_point_t timestamp, std::string playerName, std::string reason);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::ServerDroppedPlayer; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		const std::string& GetPlayerName() const { return m_PlayerName; }
		const std::string& GetReason() const { return m_Reason; }

	private:
		std::string m_PlayerName;
		std::string m_Reason;
	};
}
