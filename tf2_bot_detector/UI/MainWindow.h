#pragma once

#include "Actions/Actions.h"
#include "Actions/RCONActionManager.h"
#include "Clock.h"
#include "CompensatedTS.h"
#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"
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
#include "Application.h"
#include <mh/error/expected.hpp>

#include <optional>
#include <vector>

struct ImFont;
struct ImVec4;

namespace tf2_bot_detector
{
	class IBaseTextures;
	class IConsoleLine;
	class IConsoleLineListener;
	class ITexture;
	class ITextureManager;
	class IUpdateManager;
	class SettingsWindow;

	class MainWindow final
	{
	public:
		explicit MainWindow(TF2BDApplication* app);
		~MainWindow();

		ImFont* GetFontPointer(Font f) const;

		void DrawPlayerTooltip(IPlayer& player);
		void DrawPlayerTooltip(IPlayer& player, TeamShareResult teamShareResult, const PlayerMarks& playerAttribs);
		void DrawPlayerContextMarkMenu(const SteamID& steamid, const std::string& name, std::string& reasons);

	private:
		void OnDrawMenuBar();
		void OnDraw();
		void OnEndFrame();
		void OnDrawScoreboard();
		void OnDrawTeamStats();
		void OnDrawAllPanesDisabled();

		void OnDrawScoreboardContextMenu(IPlayer& player);
		void OnDrawScoreboardRow(IPlayer& player);
		void OnDrawColorPicker(const char* name_id, std::array<float, 4>& color);
		void OnDrawChat();
		void OnDrawServerStats();
		void DrawPlayerTooltipBody(IPlayer& player, TeamShareResult teamShareResult, const PlayerMarks& playerAttribs);

		struct ColorPicker
		{
			const char* m_Name;
			std::array<float, 4>& m_Color;
		};
		void OnDrawColorPickers(const char* id, const std::initializer_list<ColorPicker>& pickers);

		void OnDrawAppLog();

		bool b_SettingsOpen = false;
		void OnDrawSettings();
		void ToggleSettingsPopup();

		void OnDrawUpdateCheckPopup();
		bool m_UpdateCheckPopupOpen = false;
		void OpenUpdateCheckPopup();

		void OnDrawAboutPopup();
		bool m_AboutPopupOpen = false;
		void OpenAboutPopup() { m_AboutPopupOpen = true; }

		void PrintDebugInfo();
		void GenerateDebugReport();

		bool IsSleepingEnabled() const;

		void SetupFonts();

		// these fonts do not support unicode.
		ImFont* m_ProggyTiny10Font{};
		ImFont* m_ProggyTiny20Font{};
		/* m_ProggyClean13Font managed by imgui */
		ImFont* m_ProggyClean26Font{};

		// this font supports unicode.
		ImFont* m_Unifont14Font{};
		ImFont* m_Unifont24Font{};


		TF2BDApplication* m_Application;

		mh::expected<std::shared_ptr<ITexture>, std::error_condition> TryGetAvatarTexture(IPlayer& player);
		std::shared_ptr<ITextureManager> m_TextureManager;
		std::unique_ptr<IBaseTextures> m_BaseTextures;

		Settings& m_Settings;
		std::unique_ptr<SettingsWindow> m_SettingsWindow;

		/// <summary>
		/// for "sleep when unfocused" feature.
		/// note: it still will update, when a new log as been processed.
		/// </summary>
		bool b_ShouldUpdate = false;

	public:
		void QueueUpdate();
		bool ShouldUpdate();

		/// <summary>
		/// Draw our imgui menu
		///
		/// IMPORTANT NOTE: this also handles b_ShouldUpdate/QueueUpdate
		///  see: MainWindow::OnDrawAppLog
		/// </summary>
		void Draw();

		// void DrawExternal();

		void OnUpdate();
		void OnImGuiInit();
		void OpenGLInit();
	};
}
