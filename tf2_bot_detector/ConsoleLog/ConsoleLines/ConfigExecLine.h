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
	/// <summary>
	/// parses when a console script has been executed.
	///
	/// used for first spawn detection when "scout.cfg" first runs for example.
	/// </summary>
	class ConfigExecLine final : public ConsoleLineBase<ConfigExecLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ConfigExecLine(time_point_t timestamp, std::string configFileName, bool success);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::ConfigExec; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		const std::string& GetConfigFileName() const { return m_ConfigFileName; }
		bool IsSuccessful() const { return m_Success; }

	private:
		std::string m_ConfigFileName;
		bool m_Success = false;
	};
}
