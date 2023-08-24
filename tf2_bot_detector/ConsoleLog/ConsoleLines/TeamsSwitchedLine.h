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

	class TeamsSwitchedLine final : public ConsoleLineBase<TeamsSwitchedLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		TeamsSwitchedLine(time_point_t timestamp) : BaseClass(timestamp) {}
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::TeamsSwitched; }
		bool ShouldPrint() const override;
		void Print(const PrintArgs& args) const override;

		const std::string& GetConfigFileName() const { return m_ConfigFileName; }
		bool IsSuccessful() const { return m_Success; }

	private:
		std::string m_ConfigFileName;
		bool m_Success = false;
	};
}
