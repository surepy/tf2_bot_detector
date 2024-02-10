#pragma once

#include "ConfigHelpers.h"
#include "ChatWrappers.h"
#include "Clock.h"
#include "SteamID.h"

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <optional>
#include <vector>

namespace srcon
{
	class async_client;
}

namespace tf2_bot_detector
{
	enum class Font
	{
		ProggyTiny_10px,
		ProggyClean_13px, // managed by IMGUI itself
		ProggyTiny_20px,
		ProggyClean_26px,
		UniFont_14px,
		UniFont_24px
	};

	void to_json(nlohmann::json& j, const Font& d);
	void from_json(const nlohmann::json& j, Font& d);

	class IHTTPClient;
	enum class ReleaseChannel;

	void to_json(nlohmann::json& j, const ReleaseChannel& d);
	void from_json(const nlohmann::json& j, ReleaseChannel& d);

	struct AutoDetectedSettings
	{
		SteamID GetLocalSteamID() const;
		SteamID m_LocalSteamIDOverride;

		std::filesystem::path GetSteamDir() const;
		std::filesystem::path m_SteamDirOverride;

		std::filesystem::path GetTFDir() const;
		std::filesystem::path m_TFDirOverride;
	};

	struct GotoProfileSite
	{
		std::string m_Name;
		std::string m_ProfileURL;

		std::string CreateProfileURL(const SteamID& id) const;
	};

	enum class SteamAPIMode
	{
		Disabled,  // Totally opt out
		Direct,    // Requires Steam API key
	};

	enum class TFBinaryMode
	{
		x64,		// tf_win64.exe
		x86,		// tf.exe
		x86_legacy	// hl2.exe
	};

	class ISteamAPISettings
	{
	public:
		virtual ~ISteamAPISettings() = default;

		bool IsSteamAPISettingReady() const;
		virtual std::string GetSteamAPIKey() const = 0;
		virtual void SetSteamAPIKey(std::string key) = 0;
		virtual SteamAPIMode GetSteamAPIMode() const = 0;
	};

	struct GeneralSettings : public ISteamAPISettings
	{
		// custom: logging settings
		bool m_SaveApplicationLogs = true;
		bool m_SaveConsoleLogs = true;
		bool m_SaveChatHistory = true;

		// print to party chat when cheater joins
		bool m_AutoChatWarningsConnectingParty = true;
		// print to party chat the summary of current game.
		bool m_AutoChatWarningsConnectingPartyPrintSummary = true;

		// bool m_AutoChatWarningsPartyIgnore[4] = { false, false, false, false };

		// (unimplemented)
		bool m_AutoChatWarningsMarkedVSNotifications = true;

		// put kill logs in chat
		bool m_KillLogsInChat = false;

		// how many times until chat warning, in seconds
		int m_ChatWarningInterval = 10;

		// custom chat warnings (most requested feature)
		bool m_UseCustomChatWarnings = false;
		// maybe?
		// bool m_IgnoreBotLeaderPriority = false;
		std::string m_OneCheaterConnectingMessage = "Heads up! There is a known cheater joining the other team! Name unknown until they fully join.";
		std::string m_MultipleCheaterConnectingMessage = "Heads up! There are {} known cheaters joining the other team! Names unknown until they fully join.";

		std::string m_OneCheaterWarningMessage = "Attention! There is a cheater on the other team named \"{}\". Please kick them!";
		std::string m_MultipleCheaterWarningMessage = "Attention! There are {} cheaters on the other team named {}. Please kick them!";

		/// <summary>
		/// use a static password, if wanted by the user (not really recommended)
		/// </summary>
		bool m_UseRconStaticParams = false;
		std::string m_RconStaticPassword = "changeme";
		uint16_t m_RconStaticPort = 40001;

		/// <summary>
		/// use recommended launch params (I actually need contimes to not be 0 :DD)
		/// </summary>
		bool m_UseLaunchRecommendedParams = true;

		/// <summary>
		/// due to rcon design limitations, ignore team state and try to call a votekick anyways in these maps (vsh_ and ze_)
		/// </summary>
		bool m_VoteKickIgnoreTeamStateOnCertainMaps = true;

		/// <summary>
		/// how many seconds to wait for attempting a votekick
		/// </summary>
		int m_MinVoteKickInterval = 30;

		/// <summary>
		/// which binary should we run?
		/// </summary>
		TFBinaryMode m_TFBinaryMode = TFBinaryMode::x64;

		// end custom

		bool m_AutoChatWarnings = true;
		bool m_AutoChatWarningsConnecting = false;
		bool m_AutoVotekick = true;
		float m_AutoVotekickDelay = 15;
		bool m_AutoMark = true;

		bool m_SleepWhenUnfocused = true;
		bool m_AutoTempMute = true;
		bool m_AutoLaunchTF2 = false;

		bool m_LazyLoadAPIData = true;

		bool m_ConfigCompatibilityMode = true;

		std::optional<ReleaseChannel> m_ReleaseChannel;

		constexpr auto GetAutoVotekickDelay() const { return std::chrono::duration<float>(m_AutoVotekickDelay); }

		std::string GetBinaryName() const;

		std::string GetSteamAPIKey() const override;
		void SetSteamAPIKey(std::string key) override;

		SteamAPIMode m_SteamAPIMode = SteamAPIMode::Disabled;
		SteamAPIMode GetSteamAPIMode() const override { return m_SteamAPIMode; }

		bool m_EnableSteamHistoryIntegration = false;
		std::string GetSteamHistoryAPIKey() const { return m_SteamHistoryAPIKey; };
		void SetSteamHistoryAPIKey(std::string key) { m_SteamHistoryAPIKey = key; };

	protected:
		const std::string& GetSteamAPIKeyDirect() const { return m_SteamAPIKey; }
		const std::string& GetSteamHistoryAPIKeyDirect() const { return m_SteamHistoryAPIKey; }

	private:
		std::string m_SteamAPIKey;
		std::string m_SteamHistoryAPIKey;
	};

	class Settings final : public AutoDetectedSettings, public GeneralSettings, ConfigFileBase
	{
	public:
		Settings();
		~Settings();

		void LoadFile();
		bool SaveFile() const;

		// Settings that are not saved in the config file because we want them to
		// reset to these defaults when the tool is reopened
		struct Unsaved
		{
			~Unsaved();

			bool m_DebugShowCommands = false;

			uint32_t m_ChatMsgWrappersToken{};
			std::optional<ChatWrappers> m_ChatMsgWrappers;
			std::unique_ptr<srcon::async_client> m_RCONClient;

			bool m_IsLaunchedFromSteam;
			std::string m_ForwardedCommandLineArguments;

		} m_Unsaved;

		bool IsSteamAPIAvailable() const;

		std::optional<bool> m_AllowInternetUsage;
		std::shared_ptr<const IHTTPClient> GetHTTPClient() const;

		std::vector<GotoProfileSite> m_GotoProfileSites;

		struct Logging
		{
			bool m_RCONPackets = false;
			bool m_DiscordRichPresence = false;

		} m_Logging;

		struct Discord
		{
			bool m_EnableRichPresence = true;

		} m_Discord;

		struct UIState
		{
			struct MainWindow
			{
				bool m_ChatEnabled = true;
				bool m_ScoreboardEnabled = true;
				bool m_AppLogEnabled = true;
				bool m_TeamStatsEnabled = true;

			} m_MainWindow;

		} m_UIState;

		struct Theme
		{
			float m_GlobalScale = 1.0f;
			Font m_Font = Font::UniFont_14px;

			struct Colors
			{
				std::array<float, 4> m_ScoreboardCheaterBG = { 1, 0, 1, 1 };
				std::array<float, 4> m_ScoreboardSuspiciousBG = { 1, 1, 0, 1 };
				std::array<float, 4> m_ScoreboardExploiterBG = { 0, 1, 1, 1 };
				std::array<float, 4> m_ScoreboardRacistBG = { 1, 1, 1, 1 };
				std::array<float, 4> m_ScoreboardYouFG = { 0, 1, 0, 1 };
				std::array<float, 4> m_ScoreboardConnectingFG = { 1, 1, 0, 0.5f };

				std::array<float, 4> m_ScoreboardFriendlyTeamBG = { 0.19704340398311615f, 0.5180000066757202f, 0.25745877623558044f, 0.5f };
				std::array<float, 4> m_ScoreboardEnemyTeamBG = { 0.8270000219345093f, 0.42039787769317627f, 0.38951700925827026f, 0.5f };

				std::array<float, 4> m_ChatLogYouFG = { 0, 1, 0, 1 };
				std::array<float, 4> m_ChatLogFriendlyTeamFG = { 0.7f, 1, 0.7f, 1 };
				std::array<float, 4> m_ChatLogEnemyTeamFG = { 1, 0.535f, 0.5f, 1 };
			} m_Colors;

		} m_Theme;

		struct TF2Interface
		{
			uint16_t GetRandomRCONPort() const;

		private:
			uint16_t m_RCONPortMin = 40000;
			uint16_t m_RCONPortMax = 60000;

			friend void to_json(nlohmann::json& j, const TF2Interface& d);
			friend void from_json(const nlohmann::json& j, TF2Interface& d);

		} m_TF2Interface;

		struct Mods
		{
			std::vector<std::string> m_DisabledList;
		} m_Mods;

	private:
		static constexpr int SETTINGS_SCHEMA_VERSION = 3;

		void ValidateSchema(const ConfigSchemaInfo& schema) const override;
		void Deserialize(const nlohmann::json& json) override;
		void Serialize(nlohmann::json& json) const override;
		void PostLoad(bool deserialized) override;

		void AddDefaultGotoProfileSites();

		mutable std::shared_ptr<IHTTPClient> m_HTTPClient;
	};
}

MH_ENUM_REFLECT_BEGIN(tf2_bot_detector::Font)
	MH_ENUM_REFLECT_VALUE(ProggyTiny_10px)
	MH_ENUM_REFLECT_VALUE(ProggyClean_13px)
	MH_ENUM_REFLECT_VALUE(ProggyTiny_20px)
	MH_ENUM_REFLECT_VALUE(ProggyClean_26px)
	MH_ENUM_REFLECT_VALUE(UniFont_14px)
	MH_ENUM_REFLECT_VALUE(UniFont_24px)
MH_ENUM_REFLECT_END()

MH_ENUM_REFLECT_BEGIN(tf2_bot_detector::SteamAPIMode)
	MH_ENUM_REFLECT_VALUE(Disabled)
	MH_ENUM_REFLECT_VALUE(Direct)
MH_ENUM_REFLECT_END()
