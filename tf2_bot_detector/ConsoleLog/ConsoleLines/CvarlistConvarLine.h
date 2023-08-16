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
	/// convar parsing and syncing to tf2bd software.
	///
	/// NOTE: unused, remove?
	/// </summary>
	class CvarlistConvarLine final : public ConsoleLineBase<CvarlistConvarLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		CvarlistConvarLine(time_point_t timestamp, std::string name, float value, std::string flagsList, std::string helpText);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const std::string& GetConvarName() const { return m_Name; }
		float GetConvarValue() const { return m_Value; }
		const std::string& GetFlagsListString() const { return m_FlagsList; }
		const std::string& GetHelpText() const { return m_HelpText; }

		ConsoleLineType GetType() const override { return ConsoleLineType::CvarlistConvar; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		std::string m_Name;
		float m_Value;
		std::string m_FlagsList;
		std::string m_HelpText;
	};
}
