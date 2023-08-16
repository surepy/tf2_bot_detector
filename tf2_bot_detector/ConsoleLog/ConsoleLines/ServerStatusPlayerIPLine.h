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

	/// <summary>
	/// it's called "ServerStatusPlayerIPLine" but it's actually the server's ip
	/// </summary>
	class ServerStatusPlayerIPLine final : public ConsoleLineBase<ServerStatusPlayerIPLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerStatusPlayerIPLine(time_point_t timestamp, std::string localIP, std::string publicIP);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::PlayerStatusIP; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		const std::string& GetLocalIP() const { return m_LocalIP; }
		const std::string& GetPublicIP() const { return m_PublicIP; }

	private:
		std::string m_LocalIP;
		std::string m_PublicIP;
	};
}
