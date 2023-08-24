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

	class DifferingLobbyReceivedLine final : public ConsoleLineBase<DifferingLobbyReceivedLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		struct Lobby
		{
			SteamID m_LobbyID;
			uint64_t m_LobbyNumber{};
			uint32_t m_MatchID;
		};

		DifferingLobbyReceivedLine(time_point_t timestamp, const Lobby& newLobby, const Lobby& currentLobby,
			bool connectedToMatchServer, bool hasLobby, bool assignedMatchEnded);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::DifferingLobbyReceived; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		const Lobby& GetNewLobby() const { return m_NewLobby; }
		const Lobby& GetCurrentLobby() const { return m_CurrentLobby; }
		bool IsConnectedToMatchServer() const { return m_ConnectedToMatchServer; }
		bool HasLobby() const { return m_HasLobby; }
		bool HasAssignedMatchEnded() const { return m_AssignedMatchEnded; }

	private:
		Lobby m_NewLobby{};
		Lobby m_CurrentLobby{};
		bool m_ConnectedToMatchServer : 1 = false;
		bool m_HasLobby : 1 = false;
		bool m_AssignedMatchEnded : 1 = false;
	};
}
