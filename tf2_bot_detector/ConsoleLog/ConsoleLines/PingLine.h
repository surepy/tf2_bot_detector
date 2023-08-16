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

	class PingLine final : public ConsoleLineBase<PingLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		PingLine(time_point_t timestamp, uint16_t ping, std::string playerName);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::Ping; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		uint16_t GetPing() const { return m_Ping; }
		const std::string& GetPlayerName() const { return m_PlayerName; }

	private:
		uint16_t m_Ping{};
		std::string m_PlayerName;
	};
}
