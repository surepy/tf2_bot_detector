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

	class EdictUsageLine final : public ConsoleLineBase<EdictUsageLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		EdictUsageLine(time_point_t timestamp, uint16_t usedEdicts, uint16_t totalEdicts);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		uint16_t GetUsedEdicts() const { return m_UsedEdicts; }
		uint16_t GetTotalEdicts() const { return m_TotalEdicts; }

		ConsoleLineType GetType() const override { return ConsoleLineType::EdictUsage; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		uint16_t m_UsedEdicts;
		uint16_t m_TotalEdicts;
	};
}
