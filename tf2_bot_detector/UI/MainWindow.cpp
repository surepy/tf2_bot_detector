#include "MainWindow.h"
#include "DiscordRichPresence.h"
#include "Networking/GithubAPI.h"
#include "Networking/SteamAPI.h"
#include "ConsoleLog/NetworkStatus.h"
#include "Platform/Platform.h"
#include "ImGui_TF2BotDetector.h"
#include "Actions/ActionGenerators.h"
#include "BaseTextures.h"
#include "Filesystem.h"
#include "GenericErrors.h"
#include "Log.h"
#include "GameData/IPlayer.h"
#include "ReleaseChannel.h"
#include "TextureManager.h"
#include "UpdateManager.h"
#include "Util/PathUtils.h"
#include "Version.h"
#include "GlobalDispatcher.h"
#include "Networking/HTTPClient.h"
#include "SettingsWindow.h"
#include "Application.h"

#include "ConsoleLog/ConsoleLines/LobbyChangedLine.h"
#include "ConsoleLog/ConsoleLines/EdictUsageLine.h"

#include <Util/ScopeGuards.h>
#include <Util/ImguiHelpers.h>
#include <imgui.h>
#include <libzippp/libzippp.h>
#include <misc/cpp/imgui_stdlib.h>
#include <mh/math/interpolation.hpp>
#include <mh/text/case_insensitive_string.hpp>
#include <mh/text/formatters/error_code.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/text/string_insertion.hpp>
#include <mh/text/stringops.hpp>
#include <srcon/async_client.h>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>

using namespace tf2_bot_detector;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

MainWindow::MainWindow(TF2BDApplication* application) :
	m_TextureManager(ITextureManager::Create()),
	m_Application(application),
	m_Settings(application->m_Settings),
	m_SettingsWindow(std::make_unique<SettingsWindow>(m_Settings, *this))
{
	PrintDebugInfo();
}

MainWindow::~MainWindow()
{
}

void MainWindow::SetupFonts()
{
	// Add ProggyClean.ttf 24px (200%) and ProggyTiny 10px

	ImFontConfig config{};
	config.OversampleV = config.OversampleH = 1; // Bitmap fonts look bad with oversampling
	//config.MergeMode = true;

	// https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#using-custom-glyph-ranges
	ImFontGlyphRangesBuilder builder;

	// Latin default (a-z)
	builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());

	// Cyrillic support
	// Cyrillic & Cyrillic Extended-A & Cyrillic Extended-B has some funky characters
	// like "Combining Cyrillic Letter I" (U+A675)
	// but u can see it so it should be fine?
	builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

	// CJK support
	// we're using ChineseSimplified because
	// A. it's unlikely we will see someone that is using traditional Chinese
	// B. and that their name is not completely a meme.
	builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
	builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
	builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesKorean());

	// I'm not including Thai/Viet as I am too unfamiliar with the language to know what
	// character points are "safe" (you can see) and what isn't. sorry.
	// builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesThai());

	ImVector<ImWchar> ranges;
	builder.BuildRanges(&ranges);

	// load into config so our glyph loads for all fonts in this config.
	config.GlyphRanges = ranges.Data;

	// load ProggyClean font(s)
	// instead of having two copies of ProggyClean, we can just pull from the IMGUI library.
	{
		// ProggyClean13 is provided by imgui itself, we need to load it first for backwards compatibility reason.
		ImGui::GetIO().Fonts->AddFontDefault();

		// We can load ProggyClean26 the same way.
		if (!m_ProggyClean26Font)
		{
			config.GlyphOffset.y = 1;
			config.SizePixels = 26;

			m_ProggyClean26Font = ImGui::GetIO().Fonts->AddFontDefault(&config);
		}
	}

	// load ProggyTiny font(s)
	{
		if (!m_ProggyTiny10Font)
		{
			m_ProggyTiny10Font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
				IFilesystem::Get().ResolvePath("fonts/ProggyTiny.ttf", PathUsage::Read).string().c_str(),
				10, &config);
		}

		if (!m_ProggyTiny20Font)
		{
			m_ProggyTiny20Font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
				IFilesystem::Get().ResolvePath("fonts/ProggyTiny.ttf", PathUsage::Read).string().c_str(),
				20, &config);
		}
	}

	// reset so we use defaults again
	config.OversampleH = 3;
	config.GlyphOffset.y = 0;
	config.SizePixels = 0.0f;
	config.PixelSnapH = true;
	// idk why it looks kinda dark with default
	// not sure if im loading it wrong, but this works for now.
	config.RasterizerMultiply = 2.f;
	
	if (!m_Unifont14Font) {
		m_Unifont14Font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
			IFilesystem::Get().ResolvePath("fonts/unifont_jp-15.0.06.ttf", PathUsage::Read).string().c_str(),
			14, &config);
	}

	if (!m_Unifont24Font) {
		m_Unifont24Font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
			IFilesystem::Get().ResolvePath("fonts/unifont_jp-15.0.06.ttf", PathUsage::Read).string().c_str(),
			24, &config);
	}

	ImGui::GetIO().Fonts->Build();
}

void MainWindow::OnImGuiInit()
{
	ImGui::GetIO().FontGlobalScale = m_Settings.m_Theme.m_GlobalScale;
	SetupFonts();
}

void MainWindow::OpenGLInit()
{
	m_BaseTextures = IBaseTextures::Create(*m_TextureManager);
}

ImFont* MainWindow::GetFontPointer(Font f) const
{
	const auto defaultFont = ImGui::GetIO().Fonts->Fonts[0];
	switch (f)
	{
	case Font::ProggyClean_26px:
		return m_ProggyClean26Font ? m_ProggyClean26Font : defaultFont;
	case Font::ProggyTiny_10px:
		return m_ProggyTiny10Font ? m_ProggyTiny10Font : defaultFont;
	case Font::ProggyTiny_20px:
		return m_ProggyTiny20Font ? m_ProggyTiny20Font : defaultFont;
	case Font::UniFont_14px:
		return m_Unifont14Font ? m_Unifont14Font : defaultFont;
	case Font::UniFont_24px:
		return m_Unifont24Font ? m_Unifont24Font : defaultFont;
	default:
		LogError("Unknown font {}", mh::enum_fmt(f));
	case Font::ProggyClean_13px:
		return defaultFont;
	}
}

void MainWindow::OnDrawColorPicker(const char* name, std::array<float, 4>& color)
{
	if (ImGui::ColorEdit4(name, color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview))
		m_Settings.SaveFile();
}

void MainWindow::OnDrawColorPickers(const char* id, const std::initializer_list<ColorPicker>& pickers)
{
	ImGui::HorizontalScrollBox(id, [&]
		{
			for (const auto& picker : pickers)
			{
				OnDrawColorPicker(picker.m_Name, picker.m_Color);
				ImGui::SameLine();
			}
		});
}

void MainWindow::OnDrawChat()
{
	OnDrawColorPickers("ChatColorPickers",
		{
			{ "You", m_Settings.m_Theme.m_Colors.m_ChatLogYouFG },
			{ "Enemies", m_Settings.m_Theme.m_Colors.m_ChatLogEnemyTeamFG },
			{ "Friendlies", m_Settings.m_Theme.m_Colors.m_ChatLogFriendlyTeamFG },
		});

	ImGui::AutoScrollBox("##fileContents", { 0, 0 }, [&]()
		{
			if (!m_Application->GetMainState())
				return;

			ImGui::PushTextWrapPos();

			const IConsoleLine::PrintArgs args{ m_Settings, *m_Application->m_WorldState, *this };
			for (auto it = m_Application->GetMainState()->m_PrintingLines.rbegin(); it != m_Application->GetMainState()->m_PrintingLines.rend(); ++it)
			{
				assert(*it);
				(*it)->Print(args);
			}

			ImGui::PopTextWrapPos();
		});
}

void MainWindow::OnDrawAppLog()
{
	ImGui::AutoScrollBox("AppLog", { 0, 0 }, [&]()
		{
			ImGui::PushTextWrapPos();

			const void* lastLogMsg = nullptr;
			for (const LogMessage& msg : ILogManager::GetInstance().GetVisibleMsgs())
			{
				const std::tm timestamp = ToTM(msg.m_Timestamp);

				ImGuiDesktop::ScopeGuards::ID id(&msg);

				ImGui::BeginGroup();
				ImGui::TextColored({ 0.25f, 1.0f, 0.25f, 0.25f }, "[%02i:%02i:%02i]",
					timestamp.tm_hour, timestamp.tm_min, timestamp.tm_sec);

				ImGui::SameLine();
				ImGui::TextFmt({ msg.m_Color.r, msg.m_Color.g, msg.m_Color.b, msg.m_Color.a }, msg.m_Text);
				ImGui::EndGroup();

				if (auto scope = ImGui::BeginPopupContextItemScope("AppLogContextMenu"))
				{
					if (ImGui::MenuItem("Copy"))
						ImGui::SetClipboardText(msg.m_Text.c_str());
				}

				lastLogMsg = &msg;
			}

			if (m_Application->m_LastLogMessage != lastLogMsg)
			{
				m_Application->m_LastLogMessage = lastLogMsg;
				QueueUpdate();
			}

			ImGui::PopTextWrapPos();
		});
}

void MainWindow::ToggleSettingsPopup()
{
	b_SettingsOpen = !b_SettingsOpen;
}

void MainWindow::OnDrawSettings()
{
	if (!b_SettingsOpen) {
		return;
	}

	if (ImGui::Begin("Settings", &b_SettingsOpen)) {
		m_SettingsWindow->OnDraw();
		ImGui::End();
	}
}

void MainWindow::OnDrawUpdateCheckPopup()
{
	static constexpr char POPUP_NAME[] = "Check for Updates##Popup";

	static bool s_Open = false;
	if (m_UpdateCheckPopupOpen)
	{
		m_UpdateCheckPopupOpen = false;
		ImGui::OpenPopup(POPUP_NAME);
		s_Open = true;
	}
}

void MainWindow::OpenUpdateCheckPopup()
{
	m_UpdateCheckPopupOpen = true;
}

void MainWindow::OnDrawAboutPopup()
{
	static constexpr char POPUP_NAME[] = "About##Popup";

	static bool s_Open = false;
	if (m_AboutPopupOpen)
	{
		m_AboutPopupOpen = false;
		ImGui::OpenPopup(POPUP_NAME);
		s_Open = true;
	}

	ImGui::SetNextWindowSize({ 600, 450 }, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal(POPUP_NAME, &s_Open))
	{
		ImGui::PushTextWrapPos();

		ImGui::TextFmt("TF2 Bot Detector v{}\n"
			"\n"
			"Automatically detects and votekicks cheaters in Team Fortress 2 Casual.\n"
			"\n"
			"This program is free, open source software licensed under the MIT license. Full license text"
			" for this program and its dependencies can be found in the licenses subfolder next to this"
			" executable.", VERSION);

		ImGui::NewLine();

		ImGui::Text("You are currently using Sleepy's build - forked from tf2bd release 1.2.1\n(commit 44f7a803d5dce93ad4f8aa9f1fd83b4ffffae625)");
#ifdef __linux__
		ImGui::Text("- (Linux ver)");
#endif

		ImGui::NewLine();

		ImGui::Separator();
		ImGui::NewLine();

		ImGui::TextFmt("Credits");
		ImGui::Spacing();
		if (ImGui::TreeNode("Code/concept by Matt \"pazer\" Haynie"))
		{
			if (ImGui::Selectable("GitHub - PazerOP", false, ImGuiSelectableFlags_DontClosePopups))
				Shell::OpenURL("https://github.com/PazerOP");
			if (ImGui::Selectable("Twitter - @PazerFromSilver", false, ImGuiSelectableFlags_DontClosePopups))
				Shell::OpenURL("https://twitter.com/PazerFromSilver");

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Artwork/icon by S-Purple"))
		{
			if (ImGui::Selectable("Twitter (NSFW)", false, ImGuiSelectableFlags_DontClosePopups))
				Shell::OpenURL("https://twitter.com/spurpleheart");

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Documentation/moderation by Nicholas \"ClusterConsultant\" Flamel"))
		{
			if (ImGui::Selectable("GitHub - ClusterConsultant", false, ImGuiSelectableFlags_DontClosePopups))
				Shell::OpenURL("https://github.com/ClusterConsultant");

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Other Attributions"))
		{
			if (ImGui::Selectable("SourceBans information made possible by xzf from SteamHistory.net", false, ImGuiSelectableFlags_DontClosePopups))
				Shell::OpenURL("https://steamhistory.net");

			ImGui::TextFmt("\"Game Ban\" icon made by Freepik from www.flaticon.com");

			ImGui::TextFmt("\"Source Bans\" icon made by minein4");
			ImGui::TreePop();
		}

		ImGui::NewLine();
		ImGui::Separator();

		ImGui::EndPopup();
	}
}

#include "ITF2BotDetectorRenderer.h"

void MainWindow::PrintDebugInfo()
{
	DebugLog("Debug Info:"s
		<< "\n\tSteam dir:         " << m_Settings.GetSteamDir()
		<< "\n\tTF dir:            " << m_Settings.GetTFDir()
		<< "\n\tSteamID:           " << m_Settings.GetLocalSteamID()
		<< "\n\tVersion:           " << VERSION
		<< "\n\tIs CI Build:       " << std::boolalpha << (TF2BD_IS_CI_COMPILE ? true : false)
		<< "\n\tCompile Timestamp: " << __TIMESTAMP__

		<< "\n\tImgui Version:     " << ImGui::GetVersion()
		<< "\n\tRenderer Info:     " << TF2BotDetectorRendererBase::GetRenderer()->RendererInfo()

		<< "\n\tIs Debug Build:    "
#ifdef _DEBUG
		<< true
#else
		<< false
#endif

#ifdef _MSC_FULL_VER
		<< "\n\t-D _MSC_FULL_VER:  " << _MSC_FULL_VER
#endif
#if _M_X64
		<< "\n\t-D _M_X64:         " << _M_X64
#endif
#if _MT
		<< "\n\t-D _MT:            " << _MT
#endif
	);
}

void MainWindow::GenerateDebugReport() try
{
	Log("Generating debug_report.zip...");
	const auto dbgReportLocation = IFilesystem::Get().ResolvePath("debug_report.zip", PathUsage::WriteLocal);

	{
		using namespace libzippp;
		ZipArchive archive(dbgReportLocation.string());
		archive.open(ZipArchive::New);

		const auto logsDir = IFilesystem::Get().GetLogsDir();
		for (const auto& entry : std::filesystem::recursive_directory_iterator(logsDir))
		{
			if (!entry.is_regular_file())
				continue;

			const auto& path = entry.path();

			if (archive.addFile(std::filesystem::relative(path, logsDir / "..").string(), path.string()))
				Log("Added file to debug report: {}", path);
			else
				LogWarning("Failed to add file to debug report: {}", path);
		}

		if (auto err = archive.close(); err != LIBZIPPP_OK)
		{
			LogError("Failed to close debug report zip archive: close() returned {}", err);
			return;
		}
	}

	Log("Finished generating debug_report.zip ({})", dbgReportLocation);
	Shell::ExploreToAndSelect(dbgReportLocation);
}
catch (...)
{
	LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to generate debug_report.zip");
}

void MainWindow::OnDrawServerStats()
{
	ImGui::PlotLines("Edicts", [&](int idx)
		{
			return m_Application->m_EdictUsageSamples[idx].m_UsedEdicts;
		}, (int)m_Application->m_EdictUsageSamples.size(), 0, nullptr, 0, 2048);

	if (!m_Application->m_EdictUsageSamples.empty())
	{
		ImGui::SameLine(0, 4);

		auto& lastSample = m_Application->m_EdictUsageSamples.back();
		const float percent = float(lastSample.m_UsedEdicts) / lastSample.m_MaxEdicts;
		ImGui::ProgressBar(percent, { -1, 0 },
			mh::pfstr<64>("%i (%1.0f%%)", lastSample.m_UsedEdicts, percent * 100).c_str());

		ImGui::SetHoverTooltip("{} of {} ({:1.1f}%)", lastSample.m_UsedEdicts, lastSample.m_MaxEdicts, percent * 100);
	}

	if (!m_Application->m_ServerPingSamples.empty())
	{
		ImGui::PlotLines(mh::fmtstr<64>("Average ping: {}", m_Application->m_ServerPingSamples.back().m_Ping).c_str(),
			[&](int idx)
			{
				return m_Application->m_ServerPingSamples[idx].m_Ping;
			}, (int)m_Application->m_ServerPingSamples.size(), 0, nullptr, 0);
	}

	//OnDrawNetGraph();
}

void MainWindow::OnDraw()
{
	//if (m_SettingsWindow && m_SettingsWindow->ShouldClose())
	//	m_SettingsWindow.reset();

	OnDrawUpdateCheckPopup();
	OnDrawAboutPopup();

	{
		ISetupFlowPage::DrawState ds;
		ds.m_ActionManager = &m_Application->GetActionManager();
		ds.m_UpdateManager = m_Application->m_UpdateManager.get();
		ds.m_Settings = &m_Settings;

		if (m_Application->m_SetupFlow.OnDraw(m_Settings, ds))
			return;
	}

	if (!m_Application->GetMainState())
		return;

	const auto& mainWindowState = m_Settings.m_UIState.m_MainWindow;
	const bool columnsEnabled = (mainWindowState.m_ChatEnabled && (mainWindowState.m_AppLogEnabled || mainWindowState.m_ScoreboardEnabled));

	if (columnsEnabled)
		ImGui::Columns(2, "MainWindowSplit");

	ImGui::HorizontalScrollBox("SettingsScroller", [&]
		{
			ImGui::Checkbox("Pause", &m_Application->m_Paused); ImGui::SameLine();

			auto& settings = m_Settings;
			const auto ModerationCheckbox = [&settings](const char* name, bool& value, const char* tooltip)
			{
				{
					ImGuiDesktop::ScopeGuards::TextColor text({ 1, 0.5f, 0, 1 }, !value);
					if (ImGui::Checkbox(name, &value))
						settings.SaveFile();
				}

				const char* orangeReason = "";
				if (!value)
					orangeReason = "\n\nThis label is orange to highlight the fact that it is currently disabled.";

				ImGui::SameLine();
				ImGui::SetHoverTooltip("{}{}", tooltip, orangeReason);
			};

			ModerationCheckbox("Enable Chat Warnings", m_Settings.m_AutoChatWarnings, "Enables chat message warnings about cheaters.");
			ModerationCheckbox("Enable Chat Warnings (Party)", m_Settings.m_AutoChatWarningsConnectingParty, "Enables party message warnings about cheaters.");
			ModerationCheckbox("Enable Auto Votekick", m_Settings.m_AutoVotekick, "Automatically votekicks cheaters on your team.");
			ModerationCheckbox("Enable Auto-mark", m_Settings.m_AutoMark, "Automatically marks players matching the detection rules.");

			ImGui::Checkbox("Show Commands", &m_Settings.m_Unsaved.m_DebugShowCommands); ImGui::SameLine();
			ImGui::SetHoverTooltip("Prints out all game commands to the log.");
		});

#ifdef _DEBUG
	{
		ImGui::Value("Time (Compensated)", to_seconds<float>(m_Application->GetCurrentTimestampCompensated() - m_Application->m_OpenTime));

		auto leader = m_Application->GetModLogic().GetBotLeader();
		ImGui::Value("Bot Leader", leader ? mh::fmtstr<128>("{}", *leader).view() : ""sv);

		ImGui::TextFmt("Is vote in progress:");
		ImGui::SameLine();
		if (m_Application->GetWorld().IsVoteInProgress())
			ImGui::TextFmt({ 1, 1, 0, 1 }, "YES");
		else
			ImGui::TextFmt({ 0, 1, 0, 1 }, "NO");
		
		ImGui::TextFmt("FPS: {:1.1f}", 1000.0f / ImGui::GetIO().Framerate);

		ImGui::Value("Texture Count", m_TextureManager->GetActiveTextureCount());

		ImGui::TextFmt("RAM Usage: {:1.1f} MB", Platform::Processes::GetCurrentRAMUsage() / 1024.0f / 1024);

		if (auto client = m_Settings.GetHTTPClient())
		{
			const IHTTPClient::RequestCounts reqs = client->GetRequestCounts();
			ImGui::TextFmt("HTTP Requests: {} total | ", reqs.m_Total);

			ImGui::SameLineNoPad();
			ImGui::TextFmt({ 1, 0.5f, 0.5f, 1 }, "{} failed ({:1.1f}%)",
				reqs.m_Failed, reqs.m_Failed / float(reqs.m_Total) * 100);

			const auto QueuedText = [](uint32_t count, const std::string_view& name)
			{
				ImGui::SameLineNoPad();
				ImGui::TextFmt(" | ");

				ImGui::SameLineNoPad();
				ImGui::TextFmt(
					count > 0 ? ImVec4{ 2.0f / 3, 1, 2.0f / 3, 1 } : ImVec4{ 0.5f, 0.5f, 0.5f, 1 },
					"{} {}", count, name);
			};

			QueuedText(reqs.m_InProgress, "running");
			QueuedText(reqs.m_Throttled, "throttled");
		}
		else
		{
			ImGui::TextFmt("HTTP Requests: HTTPClient Unavailable");
		}
	}
#endif

	ImGui::Value("Blacklisted user count", m_Application->GetModLogic().GetBlacklistedPlayerCount());
	ImGui::Value("Rule count", m_Application->GetModLogic().GetRuleCount());

	if (m_Application->GetMainState())
	{
		auto& world = m_Application->GetWorld();
		const auto parsedLineCount = m_Application->m_ParsedLineCount;
		const auto parseProgress = m_Application->GetMainState()->m_Parser.GetParseProgress();

		if (parseProgress < 0.95f)
		{
			ImGui::ProgressBar(parseProgress, { 0, 0 }, mh::pfstr<64>("%1.2f %%", parseProgress * 100).c_str());
			ImGui::SameLine(0, 4);
		}

		ImGui::Value("Parsed line count", parsedLineCount);

		ImGui::Text("Connected To:");
		ImGui::SameLine();
		std::string hostname = world.GetServerHostName();
		if (!hostname.empty()) {
			ImGui::Text(hostname.c_str());
		}
		else {
			ImGui::TextFmt({ 1, 1, 0, 1 }, "unconnected");
		}
	}

	//OnDrawServerStats();
	if (mainWindowState.m_ChatEnabled)
		OnDrawChat();

	if (columnsEnabled)
		ImGui::NextColumn();

	if (mainWindowState.m_ScoreboardEnabled)
		OnDrawScoreboard();
	if (mainWindowState.m_AppLogEnabled)
		OnDrawAppLog();

	if (columnsEnabled)
		ImGui::Columns();

	if (!mainWindowState.m_ChatEnabled && !mainWindowState.m_ScoreboardEnabled && !mainWindowState.m_AppLogEnabled)
		OnDrawAllPanesDisabled();
}

void MainWindow::OnDrawAllPanesDisabled()
{
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::Text("All panes are currently disabled.");
	ImGui::NewLine();

	auto& mainWindowState = m_Settings.m_UIState.m_MainWindow;
	if (ImGui::Button("Show Chat"))
	{
		mainWindowState.m_ChatEnabled = true;
		m_Settings.SaveFile();
	}
	if (ImGui::Button("Show Scoreboard"))
	{
		mainWindowState.m_ScoreboardEnabled = true;
		m_Settings.SaveFile();
	}
	if (ImGui::Button("Show App Log"))
	{
		mainWindowState.m_AppLogEnabled = true;
		m_Settings.SaveFile();
	}
}

void MainWindow::OnEndFrame()
{
	m_TextureManager->EndFrame();
}

void MainWindow::OnDrawMenuBar()
{
	const bool isInSetupFlow = m_Application->m_SetupFlow.ShouldDraw();
	
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Open Config Folder"))
			Shell::ExploreTo(IFilesystem::Get().GetConfigDir());
		if (ImGui::MenuItem("Open Logs Folder"))
			Shell::ExploreTo(IFilesystem::Get().GetLogsDir());

		ImGui::Separator();

		if (!isInSetupFlow)
		{
			if (ImGui::MenuItem("Reload Playerlists/Rules"))
				m_Application->GetModLogic().ReloadConfigFiles();
			if (ImGui::MenuItem("Reload Settings"))
				m_Settings.LoadFile();

			ImGui::Separator();
		}

		if (ImGui::MenuItem("Generate Debug Report"))
			GenerateDebugReport();

		ImGui::Separator();

		//if (ImGui::MenuItem("Exit", "Alt+F4"))
		//	SetShouldClose(true);

		ImGui::EndMenu();
	}

	if (!isInSetupFlow && ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Show Chat Log", nullptr, &m_Settings.m_UIState.m_MainWindow.m_ChatEnabled))
			m_Settings.SaveFile();
		if (ImGui::MenuItem("Show App Log", nullptr, &m_Settings.m_UIState.m_MainWindow.m_AppLogEnabled))
			m_Settings.SaveFile();
		if (ImGui::MenuItem("Show Team Stats", nullptr, &m_Settings.m_UIState.m_MainWindow.m_TeamStatsEnabled))
			m_Settings.SaveFile();
		if (ImGui::MenuItem("Show Scoreboard", nullptr, &m_Settings.m_UIState.m_MainWindow.m_ScoreboardEnabled))
			m_Settings.SaveFile();

		ImGui::EndMenu();
	}

#ifdef _DEBUG
	if (ImGui::BeginMenu("Debug"))
	{
		ImGui::Separator();

		if (ImGui::MenuItem("FakeState"))
		{
			// TODO: fake a in-game state so i dont have to launch tf2 every time i want to debug this
			//  though i don't think i have the effort to do this tbf
		}

		if (ImGui::MenuItem("Crash"))
		{
			struct Test
			{
				int i;
			};

			Test* testPtr = nullptr;
			testPtr->i = 42;
		}
		ImGui::EndMenu();
	}

#ifdef _DEBUG
	static bool s_ImGuiDemoWindow = false;
#endif
	if (ImGui::BeginMenu("Window"))
	{
#ifdef _DEBUG
		ImGui::MenuItem("ImGui Demo Window", nullptr, &s_ImGuiDemoWindow);
#endif
		ImGui::EndMenu();
	}

#ifdef _DEBUG
	if (s_ImGuiDemoWindow)
		ImGui::ShowDemoWindow(&s_ImGuiDemoWindow);
#endif
#endif

	if (!isInSetupFlow || m_Application->m_SetupFlow.GetCurrentPage() == SetupFlowPage::TF2CommandLine)
	{
		//if (ImGui::MenuItem("PlayerList")) {}

		if (ImGui::MenuItem("Settings")) {
			ToggleSettingsPopup();
		}
	}

	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("Open GitHub"))
			Shell::OpenURL("https://github.com/surepy/tf2_bot_detector");
		if (ImGui::MenuItem("Open Discord"))
			Shell::OpenURL("https://discord.gg/W8ZSh3Z");

		ImGui::Separator();

		static const mh::fmtstr<128> VERSION_STRING_LABEL("Version: {}", VERSION);
		ImGui::MenuItem(VERSION_STRING_LABEL.c_str(), nullptr, false, false);

		ImGui::Separator();

		if (ImGui::MenuItem("About TF2 Bot Detector"))
			OpenAboutPopup();

		ImGui::EndMenu();
	}
}


void tf2_bot_detector::MainWindow::QueueUpdate()
{
	b_ShouldUpdate = true;
	m_Application->QueueUpdate();
}

bool tf2_bot_detector::MainWindow::ShouldUpdate()
{
	return !this->IsSleepingEnabled() || b_ShouldUpdate;
}

void MainWindow::Draw()
{
	if (ImGui::BeginMainMenuBar()) {
		this->OnDrawMenuBar();
		ImGui::EndMainMenuBar();
	}

	ImGuiWindowFlags bd_external_flags = ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;

	// make it fullscreen to our main viewport.
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushFont(GetFontPointer(m_Settings.m_Theme.m_Font));
	ImGui::GetIO().FontGlobalScale = m_Settings.m_Theme.m_GlobalScale;

	if (ImGui::Begin("TF2 Bot Detector", 0, bd_external_flags)) {
		this->OnDraw();
		ImGui::End();
	}

	this->OnDrawSettings();

	this->OnEndFrame();

	ImGui::PopFont();
}

bool MainWindow::IsSleepingEnabled() const
{
	return m_Settings.m_SleepWhenUnfocused;
}

mh::expected<std::shared_ptr<ITexture>, std::error_condition> MainWindow::TryGetAvatarTexture(IPlayer& player)
{
	using StateTask_t = mh::task<mh::expected<std::shared_ptr<ITexture>, std::error_condition>>;

	struct PlayerAvatarData
	{
		StateTask_t m_State;

		static StateTask_t LoadAvatarAsync(mh::task<Bitmap> avatarBitmapTask,
			mh::dispatcher updateDispatcher, std::shared_ptr<ITextureManager> textureManager)
		{
			const Bitmap* avatarBitmap = nullptr;

			try
			{
				avatarBitmap = &(co_await avatarBitmapTask);
			}
			catch (...)
			{
				LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to load avatar bitmap");
				co_return ErrorCode::UnknownError;
			}

			// Switch to main thread
			co_await updateDispatcher.co_dispatch();

			try
			{
				co_return textureManager->CreateTexture(*avatarBitmap);
			}
			catch (...)
			{
				LogException(MH_SOURCE_LOCATION_CURRENT(), "Failed to create avatar bitmap");
				co_return ErrorCode::UnknownError;
			}
		}
	};

	auto& avatarData = player.GetOrCreateData<PlayerAvatarData>().m_State;

	if (avatarData.empty())
	{
		auto playerPtr = player.shared_from_this();
		const auto& summary = playerPtr->GetPlayerSummary();
		if (summary)
		{
			avatarData = PlayerAvatarData::LoadAvatarAsync(
				summary->GetAvatarBitmap(m_Settings.GetHTTPClient()),
				GetDispatcher(), m_TextureManager);
		}
		else
		{
			return summary.error();
		}
	}

	if (auto data = avatarData.try_get())
		return *data;
	else
		return std::errc::operation_in_progress;
}

