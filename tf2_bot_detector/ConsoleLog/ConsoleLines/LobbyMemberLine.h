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
	/// lobby member line.
	///
	/// example: "\s+(?:(?:Member)|(Pending))\[(\d+)\] (\[.*\])\s+team = (\w+)\s+type = (\w+)"
	/// </summary>
	class LobbyMemberLine final : public ConsoleLineBase<LobbyMemberLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		LobbyMemberLine(time_point_t timestamp, const LobbyMember& lobbyMember);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const LobbyMember& GetLobbyMember() const { return m_LobbyMember; }

		ConsoleLineType GetType() const override { return ConsoleLineType::LobbyMember; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		LobbyMember m_LobbyMember;
	};
}
