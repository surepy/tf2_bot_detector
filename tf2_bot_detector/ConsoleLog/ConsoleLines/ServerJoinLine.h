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

	class ServerJoinLine final : public ConsoleLineBase<ServerJoinLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerJoinLine(time_point_t timestamp, std::string hostName, std::string mapName,
			uint8_t playerCount, uint8_t playerMaxCount, uint32_t buildNumber, uint32_t serverNumber);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::ServerJoin; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		const std::string& GetHostName() const { return m_HostName; }
		const std::string& GetMapName() const { return m_MapName; }
		uint32_t GetBuildNumber() const { return m_BuildNumber; }
		uint32_t GetServerNumber() const { return m_ServerNumber; }
		uint8_t GetPlayerCount() const { return m_PlayerCount; }
		uint8_t GetPlayerMaxCount() const { return m_PlayerMaxCount; }

	private:
		std::string m_HostName;
		std::string m_MapName;
		uint32_t m_BuildNumber{};
		uint32_t m_ServerNumber{};
		uint8_t m_PlayerCount{};
		uint8_t m_PlayerMaxCount{};
	};
}
