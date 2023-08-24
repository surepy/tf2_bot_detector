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
	/// a player's entry in the "status" command
	/// </summary>
	class ServerStatusPlayerLine final : public ConsoleLineBase<ServerStatusPlayerLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerStatusPlayerLine(time_point_t timestamp, PlayerStatus playerStatus);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const PlayerStatus& GetPlayerStatus() const { return m_PlayerStatus; }

		ConsoleLineType GetType() const override { return ConsoleLineType::PlayerStatus; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		PlayerStatus m_PlayerStatus;
	};
}
