#pragma once

#include "Actions/Actions.h"
#include "Actions/RCONActionManager.h"
#include "Clock.h"
#include "CompensatedTS.h"
#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"
#include "Config/PlayerListJSON.h"
#include "Config/Settings.h"
#include "DiscordRichPresence.h"
#include "Networking/GithubAPI.h"
#include "ModeratorLogic.h"
#include "SetupFlow/SetupFlow.h"
#include "WorldEventListener.h"
#include "WorldState.h"
#include "LobbyMember.h"
#include "PlayerStatus.h"
#include "GameData/TFConstants.h"

#include <mh/error/expected.hpp>

#include <optional>
#include <vector>
#include <memory>

namespace tf2_bot_detector
{
	class IBaseTextures;
	class IConsoleLine;
	class IConsoleLineListener;
	class ITexture;
	class ITextureManager;
	class IUpdateManager;

	namespace DB
	{
		class ITempDB;
	}

	class TF2BDApplication //: IConsoleLineListener, BaseWorldEventListener
	{
	public:
		TF2BDApplication();
		~TF2BDApplication();

		static TF2BDApplication& GetApplication();

		DB::ITempDB& GetTempDB();

	private:
		std::unique_ptr<DB::ITempDB> m_TempDB;

		/*
		// moved from "MainWindow"
		const void* m_LastLogMessage = nullptr;

		void PrintDebugInfo();
		void GenerateDebugReport();

		//void OnUpdate() override;

		//bool IsSleepingEnabled() const override;

		bool IsTimeEven() const;
		float TimeSine(float interval = 1.0f, float min = 0, float max = 1) const;

		void SetupFonts();

		// IConsoleLineListener
		//void OnConsoleLineParsed(IWorldState& world, IConsoleLine& line) override;
		//void OnConsoleLineUnparsed(IWorldState& world, const std::string_view& text) override;
		//void OnConsoleLogChunkParsed(IWorldState& world, bool consoleLinesParsed) override;
		size_t m_ParsedLineCount = 0;

		// IWorldEventListener
		//void OnChatMsg(WorldState& world, const IPlayer& player, const std::string_view& msg) override;
		//void OnUpdate(WorldState& world, bool consoleLinesUpdated) override;

		bool m_Paused = false;

		// Gets the current timestamp, but time progresses in real time even without new messages
		time_point_t GetCurrentTimestampCompensated() const;

		mh::expected<std::shared_ptr<ITexture>, std::error_condition> TryGetAvatarTexture(IPlayer& player);
		std::shared_ptr<ITextureManager> m_TextureManager;
		std::unique_ptr<IBaseTextures> m_BaseTextures;

		struct PingSample
		{
			constexpr PingSample(time_point_t timestamp, uint16_t ping) :
				m_Timestamp(timestamp), m_Ping(ping)
			{
			}

			time_point_t m_Timestamp{};
			uint16_t m_Ping{};
		};

		struct PlayerExtraData final
		{
			PlayerExtraData(const IPlayer& player) : m_Parent(&player) {}

			const IPlayer* m_Parent = nullptr;

			std::string m_pendingReason;

			time_point_t m_LastPingUpdateTime{};
			std::vector<PingSample> m_PingHistory{};
			float GetAveragePing() const;
		};

		struct EdictUsageSample
		{
			time_point_t m_Timestamp;
			uint16_t m_UsedEdicts;
			uint16_t m_MaxEdicts;
		};
		std::vector<EdictUsageSample> m_EdictUsageSamples;

		time_point_t m_OpenTime;

		void UpdateServerPing(time_point_t timestamp);
		std::vector<PingSample> m_ServerPingSamples;
		time_point_t m_LastServerPingSample{};

		Settings m_Settings;

		std::unique_ptr<IUpdateManager> m_UpdateManager;

		SetupFlow m_SetupFlow;

		std::shared_ptr<IWorldState> m_WorldState;
		std::unique_ptr<IRCONActionManager> m_ActionManager;

		IWorldState& GetWorld() { return *m_WorldState; }
		const IWorldState& GetWorld() const { return *m_WorldState; }
		IRCONActionManager& GetActionManager() { return *m_ActionManager; }
		const IRCONActionManager& GetActionManager() const { return *m_ActionManager; }

		struct PostSetupFlowState
		{
			PostSetupFlowState(MainWindow& window);

			MainWindow* m_Parent = nullptr;
			std::unique_ptr<IModeratorLogic> m_ModeratorLogic;

			ConsoleLogParser m_Parser;
			std::list<std::shared_ptr<const IConsoleLine>> m_PrintingLines;  // newest to oldest order
			static constexpr size_t MAX_PRINTING_LINES = 512;
			mh::generator<IPlayer&> GeneratePlayerPrintData();

			void OnUpdateDiscord();
#ifdef TF2BD_ENABLE_DISCORD_INTEGRATION
			std::unique_ptr<IDRPManager> m_DRPManager;
#endif
		};
		std::optional<PostSetupFlowState> m_MainState;

	public:
		IModeratorLogic& GetModLogic() { return *m_MainState.value().m_ModeratorLogic; }
		const IModeratorLogic& GetModLogic() const { return *m_MainState.value().m_ModeratorLogic; }

	*/
	};
}
