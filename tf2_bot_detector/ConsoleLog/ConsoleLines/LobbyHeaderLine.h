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
	/// lobby header line.
	///
	/// example: "CTFLobbyShared: ID:([0-9a-f]*)\s+(\d+) member\(s\), (\d+) pending"
	/// </summary>
	class LobbyHeaderLine final : public ConsoleLineBase<LobbyHeaderLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		LobbyHeaderLine(time_point_t timestamp, unsigned memberCount, unsigned pendingCount);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		auto GetMemberCount() const { return m_MemberCount; }
		auto GetPendingCount() const { return m_PendingCount; }

		ConsoleLineType GetType() const override { return ConsoleLineType::LobbyHeader; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		unsigned m_MemberCount;
		unsigned m_PendingCount;
	};
}
