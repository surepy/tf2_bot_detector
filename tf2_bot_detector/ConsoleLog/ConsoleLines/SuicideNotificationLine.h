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

	class SuicideNotificationLine final : public ConsoleLineBase<SuicideNotificationLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		SuicideNotificationLine(time_point_t timestamp, std::string name, SteamID steamid);
		SuicideNotificationLine(time_point_t timestamp, std::string name);

		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const std::string& GetName() const { return m_Name; }
		const SteamID GetId() const { return m_ID; }
		ConsoleLineType GetType() const override { return ConsoleLineType::SuicideNotification; }

		bool ShouldPrint() const override { return true; }
		void Print(const PrintArgs& args) const override;

	private:
		SteamID m_ID;
		std::string m_Name;
	};
}
