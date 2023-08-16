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

	class QueueStateChangeLine final : public ConsoleLineBase<QueueStateChangeLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		QueueStateChangeLine(time_point_t timestamp, TFMatchGroup queueType, TFQueueStateChange stateChange);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::QueueStateChange; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		TFMatchGroup GetQueueType() const { return m_QueueType; }
		TFQueueStateChange GetStateChange() const { return m_StateChange; }

	private:
		TFMatchGroup m_QueueType{};
		TFQueueStateChange m_StateChange{};
	};
}
