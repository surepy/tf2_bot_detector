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

	class ServerStatusPlayerCountLine final : public ConsoleLineBase<ServerStatusPlayerCountLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerStatusPlayerCountLine(time_point_t timestamp, uint8_t playerCount,
			uint8_t botCount, uint8_t maxPlayers);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		uint8_t GetPlayerCount() const { return m_PlayerCount; }
		uint8_t GetBotCount() const { return m_BotCount; }
		uint8_t GetMaxPlayerCount() const { return m_MaxPlayers; }

		ConsoleLineType GetType() const override { return ConsoleLineType::PlayerStatusCount; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		uint8_t m_PlayerCount;
		uint8_t m_BotCount;
		uint8_t m_MaxPlayers;
	};
}
