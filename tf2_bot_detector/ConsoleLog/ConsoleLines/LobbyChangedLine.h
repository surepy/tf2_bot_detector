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

	enum class LobbyChangeType
	{
		Created,
		Updated,
		Destroyed,
	};

	class LobbyChangedLine final : public ConsoleLineBase<LobbyChangedLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		LobbyChangedLine(time_point_t timestamp, LobbyChangeType type);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::LobbyChanged; }
		LobbyChangeType GetChangeType() const { return m_ChangeType; }
		bool ShouldPrint() const override;
		void Print(const PrintArgs& args) const override;

	private:
		LobbyChangeType m_ChangeType;
	};
}
