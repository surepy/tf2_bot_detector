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

	/// <summary>
	/// parsed tf2 chat type
	/// </summary>
	class ChatConsoleLine final : public ConsoleLineBase<ChatConsoleLine, false>
	{
		using BaseClass = ConsoleLineBase;

	public:
		ChatConsoleLine(time_point_t timestamp, std::string playerName, std::string message, bool isDead,
			bool isTeam, bool isSelf, TeamShareResult teamShare, SteamID id);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);
		//static std::shared_ptr<ChatConsoleLine> TryParseFlexible(const std::string_view& text, time_point_t timestamp);

		ConsoleLineType GetType() const override { return ConsoleLineType::Chat; }
		void Print(const PrintArgs& args) const override;

		const std::string& GetPlayerName() const { return m_PlayerName; }
		const std::string& GetMessage() const { return m_Message; }
		const SteamID getSteamID() const { return m_PlayerSteamID; }
		bool IsDead() const { return m_IsDead; }
		bool IsTeam() const { return m_IsTeam; }
		bool IsSelf() const { return m_IsSelf; }
		TeamShareResult GetTeamShareResult() const { return m_TeamShareResult; }

	private:
		//static std::shared_ptr<ChatConsoleLine> TryParse(const std::string_view& text, time_point_t timestamp, bool flexible);

		std::string m_PlayerName;
		std::string m_Message;
		SteamID m_PlayerSteamID;
		TeamShareResult m_TeamShareResult;
		bool m_IsDead : 1;
		bool m_IsTeam : 1;
		bool m_IsSelf : 1;

		// this really shouldn't be here, but can't think of a better solution atm.
		static std::string m_PendingMarkReason;
	};
}
