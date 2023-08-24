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

	class ServerStatusMapLine final : public ConsoleLineBase<ServerStatusMapLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ServerStatusMapLine(time_point_t timestamp, std::string mapName, const std::array<float, 3>& position);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const std::string& GetMapName() const { return m_MapName; }
		const std::array<float, 3>& GetPosition() const { return m_Position; }

		ConsoleLineType GetType() const override { return ConsoleLineType::PlayerStatusMapPosition; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		std::string m_MapName;
		std::array<float, 3> m_Position{};
	};
}
