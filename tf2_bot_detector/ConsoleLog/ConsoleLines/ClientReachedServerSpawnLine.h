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
	/// "Client reached server_spawn"
	/// </summary>
	class ClientReachedServerSpawnLine final : public ConsoleLineBase<ClientReachedServerSpawnLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		using ConsoleLineBase::ConsoleLineBase;
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		ConsoleLineType GetType() const override { return ConsoleLineType::ClientReachedServerSpawn; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;
	};
}
