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

	class ServerStatusHostnameLine final : public ConsoleLineBase<ServerStatusHostnameLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerStatusHostnameLine(time_point_t timestamp, std::string hostName);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const std::string& GetHostName() const { return m_HostName; }

		ConsoleLineType GetType() const override { return ConsoleLineType::PlayerStatusHostName; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override {};

	private:
		std::string m_HostName;
	};
}
