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

	class PartyHeaderLine final : public ConsoleLineBase<PartyHeaderLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		PartyHeaderLine(time_point_t timestamp, TFParty party);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const TFParty& GetParty() const { return m_Party; }

		ConsoleLineType GetType() const override { return ConsoleLineType::PartyHeader; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		TFParty m_Party{};
	};
}
