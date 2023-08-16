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

	class InQueueLine final : public ConsoleLineBase<InQueueLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		InQueueLine(time_point_t timestamp, TFMatchGroup queueType, time_point_t queueStartTime);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::InQueue; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		TFMatchGroup GetQueueType() const { return m_QueueType; }
		time_point_t GetQueueStartTime() const { return m_QueueStartTime; }

	private:
		TFMatchGroup m_QueueType{};
		time_point_t m_QueueStartTime{};
	};
}
