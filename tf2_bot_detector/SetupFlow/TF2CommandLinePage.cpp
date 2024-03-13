#include "TF2CommandLinePage.h"
#include "Config/Settings.h"
#include "Platform/Platform.h"
#include "UI/ImGui_TF2BotDetector.h"
#include "Log.h"
#include "Util/TextUtils.h"

#include "Application.h"
#include "Actions/RCONActionManager.h"

#include <mh/future.hpp>
#include <mh/text/charconv_helper.hpp>
#include <mh/text/string_insertion.hpp>
#include <mh/text/case_insensitive_string.hpp>
#include <srcon/srcon.h>
#include <vdf_parser.hpp>

#include <chrono>
#include <random>

#undef DrawState

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace tf2_bot_detector;

static std::string GenerateRandomRCONPassword(size_t length = 16)
{
	std::mt19937 generator;
	{
		std::random_device randomSeed;
		generator.seed(randomSeed());
	}

	constexpr char PALETTE[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::uniform_int_distribution<size_t> dist(0, std::size(PALETTE) - 2);

	std::string retVal(length, '\0');
	for (size_t i = 0; i < length; i++)
		retVal[i] = PALETTE[dist(generator)];

	return retVal;
}

void TF2CommandLinePage::Data::TryUpdateCmdlineArgs()
{
	if (m_CommandLineArgsTask.is_ready())
	{
		const auto& args = m_CommandLineArgsTask.get();
		m_MultipleInstances = args.size() > 1;
		if (!m_MultipleInstances)
		{
			if (args.empty())
				m_CommandLineArgs.reset();
			else
				m_CommandLineArgs = TF2CommandLine::Parse(args.at(0));
		}

		m_CommandLineArgsTask = {};
		m_AtLeastOneUpdateRun = true;
	}

	if (!m_CommandLineArgsTask.valid())
	{
		// See about starting a new update

		const auto curTime = clock_t::now();
		if (!m_AtLeastOneUpdateRun || (curTime >= (m_LastCLUpdate + CL_UPDATE_INTERVAL)))
		{
			m_CommandLineArgsTask = Processes::GetTF2CommandLineArgsAsync();
			m_LastCLUpdate = curTime;
		}
	}
}

auto TF2CommandLinePage::ValidateSettings(const Settings& settings) const -> ValidateSettingsResult
{
	if (!Processes::IsTF2Running()) {
		// we have to make m_Unsaved.m_RCONClient null or else RCONActionManager will continue to queue commands
		// until we literally run out of ports, see https://github.com/surepy/tf2_bot_detector/issues/25
		// so we just break all the promises and clear our rcon client, as it doesn't matter and having it around is actually worse.
		if (settings.m_Unsaved.m_RCONClient) {
			// clear any actions that were about to be commited
			TF2BDApplication::GetApplication().GetActionManager().clearActions();
			// delete our rcon client.
			const_cast<Settings&>(settings).m_Unsaved.m_RCONClient.reset();
		}

		return ValidateSettingsResult::TriggerOpen;
	}
	if (!m_Data.m_CommandLineArgs.has_value() || !m_Data.m_CommandLineArgs->IsPopulated()) {
		return ValidateSettingsResult::TriggerOpen;
	}

	return ValidateSettingsResult::Success;
}

auto TF2CommandLinePage::TF2CommandLine::Parse(const std::string_view& cmdLine) -> TF2CommandLine
{
	const auto args = Shell::SplitCommandLineArgs(cmdLine);

	TF2CommandLine cli{};
	cli.m_FullCommandLine = cmdLine;

	for (size_t i = 0; i < args.size(); i++)
	{
		if (i < (args.size() - 1))
		{
			// We have at least one more arg
			if (cli.m_IP.empty() && args[i] == "+ip")
			{
				cli.m_IP = args[++i];
				continue;
			}
			else if (cli.m_RCONPassword.empty() && args[i] == "+rcon_password")
			{
				cli.m_RCONPassword = args[++i];
				continue;
			}
			else if (!cli.m_RCONPort.has_value() && args[i] == "+hostport")
			{
				if (uint16_t parsedPort; mh::from_chars(args[i + 1], parsedPort))
				{
					cli.m_RCONPort = parsedPort;
					i++;
					continue;
				}
			}
			else if (args[i] == "-usercon")
			{
				cli.m_UseRCON = true;
				continue;
			}
		}
	}

	return cli;
}

bool TF2CommandLinePage::TF2CommandLine::IsPopulated() const
{
	if (!m_UseRCON)
		return false;
	if (m_IP.empty())
		return false;
	if (m_RCONPassword.empty())
		return false;
	if (!m_RCONPort.has_value())
		return false;

	return true;
}

// Find the launch options that the user has configured through steam gui
static std::string FindUserLaunchOptions(const Settings& settings)
{
	const SteamID steamID = settings.GetLocalSteamID();
	const std::filesystem::path configPath = settings.GetSteamDir() / "userdata" / std::to_string(steamID.GetAccountID()) / "config/localconfig.vdf";
	if (!std::filesystem::exists(configPath))
	{
		LogError("Unable to find Steam config file for Steam ID {}\n\nTried looking in {}", steamID, configPath);
		return {};
	}

	DebugLog("Attempting to parse user-specified TF2 command line args from {}", configPath);

	try
	{
		std::ifstream file;
		file.exceptions(std::ios::badbit | std::ios::failbit);
		file.open(configPath);
		tyti::vdf::object localConfigRoot = tyti::vdf::read(file);

		std::shared_ptr<tyti::vdf::object> child;

		const auto FindNextChild = [&](const std::string& name)
		{
			const auto begin = child ? child->childs.begin() : localConfigRoot.childs.begin();
			const auto end = child ? child->childs.end() : localConfigRoot.childs.end();
			for (auto it = begin; it != end; ++it)
			{
				if (mh::case_insensitive_compare(it->first, name))
				{
					child = it->second;
					return true;
				}
			}

			LogError(MH_SOURCE_LOCATION_CURRENT(), "Unable to find \"{}\" key in {}", name, configPath);
			return false;
		};

		if (!FindNextChild("Software") ||
			!FindNextChild("Valve") ||
			!FindNextChild("Steam") ||
			!FindNextChild("Apps") ||
			!FindNextChild("440"))
		{
			// There was a missing key or other error
			return {};
		}

		auto keyIter = child->attribs.find("LaunchOptions");
		if (keyIter == child->attribs.end())
		{
			DebugLog("Didn't find any user-specified TF2 command line args in {}", configPath);
			return {}; // User has never set any launch options
		}

		DebugLog("Found user-specified TF2 command line args in {}: {}", configPath, std::quoted(keyIter->second));
		return keyIter->second; // Return launch options
	}
	catch (const std::exception&)
	{
		LogException("Failed to determine user-specified TF2 command line arguments");
		return {};
	}
}

/// <summary>
/// Actually launch tf2 with the necessary command line args for tf2bd to communicate with it
///
/// Fun Note: you don't need to use this button, just launch your tf2 with
/// "-usercon +sv_rcon_whitelist_address 127.0.0.1 +ip 0.0.0.0 +hostport [port] +rcon_password [pass] +net_start"
/// </summary>
/// <param name="settings"></param>
/// <param name="rconPassword"></param>
/// <param name="rconPort"></param>
static void OpenTF2(const Settings& settings, const std::string_view& rconPassword, uint16_t rconPort)
{
	const std::filesystem::path hl2Path = settings.GetTFDir() / ".." / settings.GetBinaryName();

	if (!std::filesystem::exists(hl2Path)) {
		LogError("Can't open this file! (empty() returned true) path={}", hl2Path);
		return;
	}

	std::string args = settings.m_Unsaved.m_IsLaunchedFromSteam ? settings.m_Unsaved.m_ForwardedCommandLineArguments : FindUserLaunchOptions(settings);

	// TODO: scrub any conflicting alias or one-time-use commands from this
	// required args
	args <<
		" bd" // Dummy option in case user has mismatched command line args in their steam config
		" -game tf"
		" -steam -secure"  // One or both of these is needed when launching the game directly
		" -usercon"
		" +developer 1"
		" +ip 0.0.0.0"
		" +sv_rcon_whitelist_address 127.0.0.1"
		" +sv_quota_stringcmdspersecond 1000000" // workaround for mastercomfig causing crashes on local servers
		" +rcon_password " << rconPassword <<
		" +hostport " << rconPort <<
		" +net_start"
		" +con_timestamp 1"
		" -condebug"
		" -conclearlog"
		;


#ifdef __linux__
	if (args.length() > 437) {
		LogWarning("forcing m_UseLaunchRecommendedParams=false as args is too long to fit!", args.length());
	}

	// 437 is the max we can go before tf2 doesn't launch (512 - RecommendedParams.length)
	if (settings.m_UseLaunchRecommendedParams && args.length() <= 437)
#else
	if (settings.m_UseLaunchRecommendedParams) 
#endif
	{
		args
			<< " +alias cl_reload_localization_files" // This command reloads files in backwards order, so any customizations get overwritten by stuff from the base game
			<< " +alias developer" // disables the "developer" command
			<< " +contimes 0" // the text in the top left when developer >= 1
			<< " +alias ip"; // disables the "ip" command
	}

#ifdef __linux__
	if (args.length() > 512) {
		// the game will not launch, but let's try anyway.
		LogWarning("args length is >512! (={}) the game might not launch!", args.length());
	}

	// bad fix
	char* library_path = getenv("LD_LIBRARY_PATH");
	const std::filesystem::path libPath32 = settings.GetTFDir() / ".." / "bin";
	const std::filesystem::path libPath64 = settings.GetTFDir() / ".." / "bin" / "linux64";

	std::string new_library_path = fmt::format("{}:{}:$LD_LIBRARY_PATH", libPath32.string(), libPath64.string());
	Log("[Linux] new LD_LIBRARY_PATH = {}", new_library_path);
	setenv("LD_LIBRARY_PATH", new_library_path.c_str(), true);
	setenv("SteamEnv", "1", true);
#endif
	Processes::Launch(hl2Path, args);
#ifdef __linux__
	if (library_path) {
		setenv("LD_LIBRARY_PATH", library_path, true);
	}
#endif
}

TF2CommandLinePage::RCONClientData::RCONClientData(std::string pwd, uint16_t port) :
	m_Client(std::make_unique<srcon::async_client>())
{
	m_Client->set_logging(true, true); // TEMP: turn on logging

	srcon::srcon_addr addr;
	addr.addr = "127.0.0.1";
	addr.pass = std::move(pwd);
	addr.port = port;

	m_Client->set_addr(std::move(addr));

	m_Message = "Connecting...";
}

bool TF2CommandLinePage::RCONClientData::Update()
{
	if (!m_Success)
	{
		if (mh::is_future_ready(m_Future))
		{
			try
			{
				m_Message = m_Future.get();
				m_MessageColor = { 0, 1, 0, 1 };
				m_Success = true;
			}
			catch (const srcon::srcon_error& e)
			{
				DebugLog(MH_SOURCE_LOCATION_CURRENT(), e.what());

				using srcon::srcon_errc;
				switch (e.get_errc())
				{
				case srcon_errc::bad_rcon_password:
					m_MessageColor = { 1, 0, 0, 1 };
					m_Message = "Bad rcon password, this should never happen!";
					break;
				case srcon_errc::rcon_connect_failed:
					m_MessageColor = { 1, 1, 0.5, 1 };
					m_Message = "TF2 not yet accepting RCON connections. Retrying...";
					break;
				case srcon_errc::socket_send_failed:
					m_MessageColor = { 1, 1, 0.5, 1 };
					m_Message = "TF2 not yet accepting RCON commands...";
					break;
				default:
					m_MessageColor = { 1, 1, 0, 1 };
					m_Message = mh::format("Unexpected error: {}", e.what());
					break;
				}
				m_Future = {};
			}
			catch (const std::exception& e)
			{
				DebugLogWarning(MH_SOURCE_LOCATION_CURRENT(), e.what());
				m_MessageColor = { 1, 0, 0, 1 };
				m_Message = mh::format("RCON connection unsuccessful: {}", e.what());
				m_Future = {};
			}
		}

		if (!m_Future.valid())
			m_Future = m_Client->send_command_async("echo [TF2BD] RCON connection successful.", false);
	}

	ImGui::TextFmt(m_MessageColor, m_Message);

	return m_Success;
}

void TF2CommandLinePage::DrawAutoLaunchTF2Checkbox(const DrawState& ds)
{
	if (tf2_bot_detector::AutoLaunchTF2Checkbox(ds.m_Settings->m_AutoLaunchTF2))
		ds.m_Settings->SaveFile();
}

void tf2_bot_detector::TF2CommandLinePage::DrawTF2LaunchMode(const DrawState& ds)
{
	// i love tech tebt
	const auto GetTFBinaryModeString = [](TFBinaryMode mode) {
		switch (mode) {
		case TFBinaryMode::x64:  return "64-bit";
		case TFBinaryMode::x86:  return "32-bit";
		case TFBinaryMode::x86_legacy:  return "32-bit (legacy)";
		}
		return "64-bit (unknown value)";
	};

	if (ImGui::BeginCombo("TF2 Binary Mode", GetTFBinaryModeString(ds.m_Settings->m_TFBinaryMode)))
	{
		const auto ModeSelectable = [&](TFBinaryMode mode) {
			if (ImGui::Selectable(GetTFBinaryModeString(mode), ds.m_Settings->m_TFBinaryMode == mode)) {
				ds.m_Settings->m_TFBinaryMode = mode;
				ds.m_Settings->SaveFile();
			}
		};

		ModeSelectable(TFBinaryMode::x64);
		ModeSelectable(TFBinaryMode::x86);
		ModeSelectable(TFBinaryMode::x86_legacy);

		ImGui::EndCombo();
	}
}

void TF2CommandLinePage::DrawRconStaticParamsCheckbox(const DrawState& ds)
{
	if (ImGui::Checkbox("Use Static Rcon Launch Parameters (Not Recommended)", &ds.m_Settings->m_UseRconStaticParams))
		ds.m_Settings->SaveFile();

	if (ds.m_Settings->m_UseRconStaticParams) {
		if (ImGui::InputText("password", &ds.m_Settings->m_RconStaticPassword, ImGuiInputTextFlags_CharsNoBlank)) {
			ds.m_Settings->SaveFile();
		}

		// imgui stuff so it's not a scalar input for ports
		std::string port = std::to_string(ds.m_Settings->m_RconStaticPort);

		// this code has some flaws,
		// 1. it doesn't check if the port given is even a valid port
		// 2. it doesnt check if that port is available
		// however, we can blame the user for using this option so lol
		if (ImGui::InputText("port", &port, ImGuiInputTextFlags_CharsDecimal)) {
			for (size_t i = 0; i < port.size(); ++i) {
				if (!isdigit(port.at(i))) {
					port.erase(i);
				}
			}

			ds.m_Settings->m_RconStaticPort = std::stoi(port);

			ds.m_Settings->SaveFile();
		}
	}
}

void TF2CommandLinePage::DrawLaunchTF2Button(const DrawState& ds)
{
	const auto curTime = clock_t::now();
	const bool canLaunchTF2 = (curTime - m_Data.m_LastTF2LaunchTime) >= 2500ms;

	ImGui::NewLine();
	ImGui::EnabledSwitch(m_Data.m_AtLeastOneUpdateRun && canLaunchTF2, [&]
		{
			if ((ImGui::Button("Launch TF2") || (m_IsAutoLaunchAllowed && ds.m_Settings->m_AutoLaunchTF2)) && canLaunchTF2)
			{
				if (Platform::Processes::IsTF2Running())
					LogError("TF2 already running!");

				if (ds.m_Settings->m_UseRconStaticParams) {
					m_Data.m_RandomRCONPassword = ds.m_Settings->m_RconStaticPassword;
					m_Data.m_RandomRCONPort = ds.m_Settings->m_RconStaticPort;
				}
				else {
					m_Data.m_RandomRCONPassword = GenerateRandomRCONPassword();
					m_Data.m_RandomRCONPort = ds.m_Settings->m_TF2Interface.GetRandomRCONPort();
				}

				OpenTF2(*ds.m_Settings, m_Data.m_RandomRCONPassword, m_Data.m_RandomRCONPort);
				m_Data.m_LastTF2LaunchTime = curTime;
			}

			m_IsAutoLaunchAllowed = false;

		}, "Finding command line arguments...");

	ImGui::NewLine();
	ImGui::Indent();

	DrawAutoLaunchTF2Checkbox(ds);
	DrawTF2LaunchMode(ds);
	DrawRconStaticParamsCheckbox(ds);
}

void TF2CommandLinePage::DrawCommandLineArgsInvalid(const DrawState& ds, const TF2CommandLine& args)
{
	m_Data.m_TestRCONClient.reset();

	if (args.m_FullCommandLine.empty())
	{
		ImGui::TextFmt({ 1, 1, 0, 1 }, "Failed to get TF2 command line arguments.");
		ImGui::NewLine();
		ImGui::TextFmt("This might be caused by TF2 or Steam running as administrator. Remove any"
			" compatibility options that are causing TF2 or Steam to require administrator and try again.");
	}
	else
	{
		ImGui::TextFmt({ 1, 1, 0, 1 }, "Invalid TF2 command line arguments.");
		ImGui::NewLine();
		ImGui::TextFmt("TF2 must be launched via TF2 Bot Detector. Please close it, then open it again with the button below.");
		ImGui::EnabledSwitch(false, [&] { DrawLaunchTF2Button(ds); }, "TF2 is currently running. Please close it first.");

		ImGui::NewLine();

		ImGuiDesktop::ScopeGuards::GlobalAlpha alpha(0.65f);
		ImGui::TextFmt("Details:");
		ImGui::TextFmt("- Instance:");
		ImGui::SameLine();
		ImGui::TextFmt(args.m_FullCommandLine);

		ImGuiDesktop::ScopeGuards::Indent indent;

		if (!args.m_UseRCON)
			ImGui::TextFmt({ 1, 0, 0, 1 }, "Unable to find \"-usercon\"");
		else
			ImGui::TextFmt({ 0, 1, 0, 1 }, "-usercon");

		if (args.m_IP.empty())
			ImGui::TextFmt({ 1, 0, 0, 1 }, "Unable to find \"+ip\"");
		else
			ImGui::TextFmt({ 0, 1, 0, 1 }, "+ip {}", args.m_IP);

		if (args.m_RCONPassword.empty())
			ImGui::TextFmt({ 1, 0, 0, 1 }, "Unable to find \"+rcon_password\"");
		else
			ImGui::TextFmt({ 0, 1, 0, 1 }, "+rcon_password {}", args.m_RCONPassword);

		if (!args.m_RCONPort.has_value())
			ImGui::TextFmt({ 1, 0, 0, 1 }, "Unable to find \"+hostport\"");
		else
			ImGui::TextFmt({ 0, 1, 0, 1 }, "+hostport {}", args.m_RCONPort.value());
	}
}

auto TF2CommandLinePage::OnDraw(const DrawState& ds) -> OnDrawResult
{
	m_Data.TryUpdateCmdlineArgs();

	if (!m_Data.m_CommandLineArgs.has_value())
	{
		if (Platform::Processes::IsTF2Running())
		{
			ImGui::TextFmt("TF2 is currently running. Fetching command line arguments...");
		}
		else
		{
			m_Data.m_TestRCONClient.reset();
			ImGui::TextFmt("TF2 must be launched via TF2 Bot Detector. You can open it by clicking the button below.");

			DrawLaunchTF2Button(ds);

			if (ds.m_Settings->m_Unsaved.m_IsLaunchedFromSteam) {
				ImGui::InputText("Your forwarded command-line arguments", &ds.m_Settings->m_Unsaved.m_ForwardedCommandLineArguments, ImGuiInputTextFlags_ReadOnly);
			}
			else {
				ImGui::Text("Your Steam command line arguments for TF2 will be forwarded.");
			}
		}
	}
	else if (m_Data.m_MultipleInstances)
	{
		m_IsAutoLaunchAllowed = false; // Multiple instances already running for some reason, disable auto launching
		m_Data.m_TestRCONClient.reset();
		ImGui::TextFmt("More than one instance of Team Fortress 2 found. Please close the other instances.");

		ImGui::EnabledSwitch(false, [&] { DrawLaunchTF2Button(ds); }, "TF2 is currently running. Please close it first.");
	}
	else if (auto& args = m_Data.m_CommandLineArgs.value(); !args.IsPopulated())
	{
		m_IsAutoLaunchAllowed = false; // Invalid command line arguments, disable auto launching
		m_Data.m_TestRCONClient.reset();
		DrawCommandLineArgsInvalid(ds, args);
	}
	else if (!m_Data.m_RCONSuccess)
	{
		auto& args = m_Data.m_CommandLineArgs.value();
		ImGui::TextFmt("Connecting to TF2 on 127.0.0.1:{} with password {}...",
			args.m_RCONPort.value(), std::quoted(args.m_RCONPassword));

		if (!m_Data.m_TestRCONClient) {
			m_Data.m_TestRCONClient.emplace(args.m_RCONPassword, args.m_RCONPort.value());
		}

		ImGui::NewLine();
		m_Data.m_RCONSuccess = m_Data.m_TestRCONClient.value().Update();

		ImGui::NewLine();
		DrawAutoLaunchTF2Checkbox(ds);
	}
	else
	{
		return OnDrawResult::EndDrawing;
	}

	return OnDrawResult::ContinueDrawing;
}

void TF2CommandLinePage::Init(const InitState& is)
{
	m_Data = {};
}

void TF2CommandLinePage::Commit(const CommitState& cs)
{
	m_IsAutoLaunchAllowed = false;
	cs.m_Settings.m_Unsaved.m_RCONClient.reset();
	cs.m_Settings.m_Unsaved.m_RCONClient = std::move(m_Data.m_TestRCONClient.value().m_Client);
	m_Data.m_TestRCONClient.reset();
}
