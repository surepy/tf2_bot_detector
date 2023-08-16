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

	class KillNotificationLine final : public ConsoleLineBase<KillNotificationLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		KillNotificationLine(time_point_t timestamp, std::string attackerName,
			std::string victimName, std::string weaponName, bool wasCrit);
		KillNotificationLine(time_point_t timestamp, std::string attackerName, SteamID attacker,
			std::string victimName, SteamID victim, std::string weaponName, bool wasCrit);
		static std::shared_ptr<IConsoleLine> TryParse(const ConsoleLineTryParseArgs& args);

		const std::string& GetVictimName() const { return m_VictimName; }
		const SteamID GetVictim() const { return m_Victim; }
		const std::string& GetAttackerName() const { return m_AttackerName; }
		const SteamID GetAttacker() const { return m_Attacker; }
		const std::string& GetWeaponName() const { return m_WeaponName; }
		bool WasCrit() const { return m_WasCrit; }

		ConsoleLineType GetType() const override { return ConsoleLineType::KillNotification; }
		bool ShouldPrint() const override { return true; }
		void Print(const PrintArgs& args) const override;

	private:
		SteamID m_Attacker;
		SteamID m_Victim;
		std::string m_AttackerName;
		std::string m_VictimName;
		std::string m_WeaponName;
		bool m_WasCrit;
	};
}
