#pragma once

#include <mh/reflection/enum.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace tf2_bot_detector
{
	enum class KickReason;
	enum class LobbyMemberTeam : uint8_t;
	enum class PlayerAttribute;
	enum class TeamShareResult;
	class IPlayer;
	struct ModerationRule;
	struct PlayerAttributesList;
	struct PlayerMarks;
	class IRCONActionManager;
	class Settings;
	class SteamID;
	class IWorldState;
	class PlayerListJSON;

	struct VoteCooldown
	{
		duration_t m_Elapsed;
		duration_t m_Total;

		duration_t GetRemainingDuration() const;
		float GetProgress() const;
	};

	enum class AttributePersistence
	{
		Saved = (1 << 0),
		Transient = (1 << 1),

		Any = Saved | Transient,
	};

	struct MarkedFriends
	{
		std::unordered_map<PlayerAttribute, uint32_t> m_MarkedFriendsCount;
		uint32_t m_MarkedFriendsCountTotal = 0;
		uint32_t m_FriendsCountTotal = 0;
	};

	class IModeratorLogic
	{
	public:
		virtual ~IModeratorLogic() = default;

		static std::unique_ptr<IModeratorLogic> Create(IWorldState& world, const Settings& settings, IRCONActionManager& actionManager);

		virtual void Update() = 0;

		virtual bool InitiateVotekick(const IPlayer& player, KickReason reason, const PlayerMarks* marks = nullptr) = 0;

		virtual PlayerMarks GetPlayerAttributes(const SteamID& id) const = 0;
		virtual PlayerMarks HasPlayerAttributes(const SteamID& id, const PlayerAttributesList& attributes,
			AttributePersistence persistence = AttributePersistence::Any) const = 0;

		virtual bool SetPlayerAttribute(const IPlayer& id, PlayerAttribute markType, AttributePersistence persistence, bool set = true, std::string proof = "") = 0;
		virtual bool SetPlayerAttribute(const SteamID& id, std::string name, PlayerAttribute markType, AttributePersistence persistence, bool set = true, std::string proof = "") = 0;

		virtual TeamShareResult GetTeamShareResult(const SteamID& id) const = 0;

		virtual const IPlayer* GetBotLeader() const = 0;

		virtual bool IsUserRunningTool(const SteamID& id) const = 0;
		virtual void SetUserRunningTool(const SteamID& id, bool isUserRunningTool = true) = 0;

		virtual size_t GetBlacklistedPlayerCount() const = 0;
		virtual size_t GetRuleCount() const = 0;

		virtual MarkedFriends GetMarkedFriendsCount(IPlayer& id) const = 0;

		virtual void ReloadConfigFiles() = 0;

		virtual PlayerListJSON* GetPlayerList() = 0;
	};
}

MH_ENUM_REFLECT_BEGIN(tf2_bot_detector::AttributePersistence)
	MH_ENUM_REFLECT_VALUE(Saved)
	MH_ENUM_REFLECT_VALUE(Transient)
	MH_ENUM_REFLECT_VALUE(Any)
MH_ENUM_REFLECT_END()
