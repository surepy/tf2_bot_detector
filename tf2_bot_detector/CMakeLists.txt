cmake_minimum_required(VERSION 3.16)

include(GenerateExportHeader)

if (WIN32)
	add_library(tf2_bot_detector SHARED)
	set_target_properties(tf2_bot_detector PROPERTIES PDB_NAME "tf2_bot_detector_dll")

	generate_export_header(tf2_bot_detector
		EXPORT_FILE_NAME "tf2_bot_detector_export.h"
	)

	add_executable(tf2_bot_detector_launcher WIN32
		"Platform/Windows/CrashHandler.cpp"
		"Launcher/main.cpp"
		"Launcher/Resources.rc"
	)
	target_compile_definitions(tf2_bot_detector_launcher PRIVATE TF2BD_LAUNCHER_USE_WINMAIN)

	# to be able to just use codecvt stuff without windows screaming at us.
	# don't feel like using MultiByteToWideChar :) -msse2
	target_compile_definitions(tf2_bot_detector PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

	# because we're still on an old version of fmt, microsoft will scream at you (and it's very loud!)
	# remove me when we update fmt to 10.1.1.
	target_compile_definitions(tf2_bot_detector PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

	target_include_directories(tf2_bot_detector_launcher PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
	target_link_libraries(tf2_bot_detector_launcher PRIVATE tf2_bot_detector)
	set_target_properties(tf2_bot_detector_launcher PROPERTIES OUTPUT_NAME "tf2_bot_detector")
	target_compile_features(tf2_bot_detector_launcher PUBLIC cxx_std_17)
else()
	add_executable(tf2_bot_detector
		"Launcher/main.cpp"
	)

	# https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#iso.2017.par2ts
	# Note 3: The Parallel Algorithms have an external dependency on Intel TBB 2018 or later. 
	# If the <execution> header is included then -ltbb must be used to link to TBB. 
	find_package(TBB REQUIRED tbb)
	message(STATUS "Installed TBB Version: ${TBB_VERSION}")
	if (TBB_VERSION_MAJOR VERSION_LESS "2018")
   		message(FATAL_ERROR "This program requires tbb 2018 or higher.")
	endif()

	# in some platforms TBB_IMPORTED_TARGETS = nothing cuz ???
	target_link_libraries(tf2_bot_detector PUBLIC TBB::tbb TBB::tbbmalloc)
	target_link_options(tf2_bot_detector PRIVATE -ltbb)

	# warnings don't exist if you pretend they don't exist
	target_compile_options(tf2_bot_detector PRIVATE -w)

	# ya
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_compile_definitions(tf2_bot_detector PRIVATE _DEBUG)
	endif()
endif()

target_link_libraries(tf2_bot_detector PRIVATE tf2_bot_detector_renderer)

target_compile_features(tf2_bot_detector PUBLIC cxx_std_20)
set_target_properties(tf2_bot_detector PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/staging"
)

target_compile_definitions(tf2_bot_detector PRIVATE WIN32_LEAN_AND_MEAN)
target_include_directories(tf2_bot_detector PRIVATE
	"."
	"${CMAKE_CURRENT_BINARY_DIR}"
)

configure_file(Version.base.h Version.h)

target_sources(tf2_bot_detector PRIVATE
	"Actions/RCONActionManager.cpp"
	"Actions/RCONActionManager.h"
	"Actions/ActionGenerators.cpp"
	"Actions/ActionGenerators.h"
	"Actions/Actions.cpp"
	"Actions/Actions.h"
	"Actions/IActionManager.h"
	"Actions/ICommandSource.h"
	"Config/AccountAges.cpp"
	"Config/AccountAges.h"
	"Config/ConfigHelpers.cpp"
	"Config/ConfigHelpers.h"
	"Config/DRPInfo.cpp"
	"Config/DRPInfo.h"
	"Config/PlayerListJSON.cpp"
	"Config/PlayerListJSON.h"
	"Config/Rules.cpp"
	"Config/Rules.h"
	"Config/Settings.cpp"
	"Config/Settings.h"
	"ConsoleLog/ConsoleLogParser.h"
	"ConsoleLog/ConsoleLogParser.cpp"
	"ConsoleLog/ConsoleLines.cpp"
	"ConsoleLog/IConsoleLine.h"
	"ConsoleLog/ConsoleLines/GenericConsoleLine.cpp"
	"ConsoleLog/ConsoleLines/GenericConsoleLine.h"
	"ConsoleLog/ConsoleLines/ChatConsoleLine.cpp"
	"ConsoleLog/ConsoleLines/ChatConsoleLine.h"
	"ConsoleLog/ConsoleLines/LobbyHeaderLine.cpp"
	"ConsoleLog/ConsoleLines/LobbyHeaderLine.h"
	"ConsoleLog/ConsoleLines/LobbyMemberLine.cpp"
	"ConsoleLog/ConsoleLines/LobbyMemberLine.h"
	"ConsoleLog/ConsoleLines/LobbyStatusFailedLine.cpp"
	"ConsoleLog/ConsoleLines/LobbyStatusFailedLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusPlayerLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusPlayerLine.h"
	"ConsoleLog/ConsoleLines/KillNotificationLine.cpp"
	"ConsoleLog/ConsoleLines/KillNotificationLine.h"
	"ConsoleLog/ConsoleLines/ClientReachedServerSpawnLine.cpp"
	"ConsoleLog/ConsoleLines/ClientReachedServerSpawnLine.h"
	"ConsoleLog/ConsoleLines/PartyHeaderLine.cpp"
	"ConsoleLog/ConsoleLines/PartyHeaderLine.h"
	"ConsoleLog/ConsoleLines/SVCUserMessageLine.cpp"
	"ConsoleLog/ConsoleLines/SVCUserMessageLine.h"
	"ConsoleLog/ConsoleLines/ConfigExecLine.cpp"
	"ConsoleLog/ConsoleLines/ConfigExecLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusPlayerIPLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusPlayerIPLine.h"
	"ConsoleLog/ConsoleLines/QueueStateChangeLine.cpp"
	"ConsoleLog/ConsoleLines/QueueStateChangeLine.h"
	"ConsoleLog/ConsoleLines/DifferingLobbyReceivedLine.cpp"
	"ConsoleLog/ConsoleLines/DifferingLobbyReceivedLine.h"
	"ConsoleLog/ConsoleLines/ServerDroppedPlayerLine.cpp"
	"ConsoleLog/ConsoleLines/ServerDroppedPlayerLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusShortPlayerLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusShortPlayerLine.h"
	"ConsoleLog/ConsoleLines/LobbyChangedLine.cpp"
	"ConsoleLog/ConsoleLines/LobbyChangedLine.h"
	"ConsoleLog/ConsoleLines/InQueueLine.cpp"
	"ConsoleLog/ConsoleLines/InQueueLine.h"
	"ConsoleLog/ConsoleLines/ServerJoinLine.cpp"
	"ConsoleLog/ConsoleLines/ServerJoinLine.h"
	"ConsoleLog/ConsoleLines/ConnectingLine.cpp"
	"ConsoleLog/ConsoleLines/ConnectingLine.h"
	"ConsoleLog/ConsoleLines/GameQuitLine.cpp"
	"ConsoleLog/ConsoleLines/GameQuitLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusPlayerCountLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusPlayerCountLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusMapLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusMapLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusHostNameLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusHostNameLine.h"
	"ConsoleLog/ConsoleLines/SuicideNotificationLine.cpp"
	"ConsoleLog/ConsoleLines/SuicideNotificationLine.h"
	"ConsoleLog/ConsoleLines/PingLine.cpp"
	"ConsoleLog/ConsoleLines/PingLine.h"
	"ConsoleLog/ConsoleLines/ServerStatusMapLine.cpp"
	"ConsoleLog/ConsoleLines/ServerStatusMapLine.h"
	"ConsoleLog/ConsoleLines/TeamsSwitchedLine.cpp"
	"ConsoleLog/ConsoleLines/TeamsSwitchedLine.h"
	"ConsoleLog/ConsoleLines/HostNewGameLine.cpp"
	"ConsoleLog/ConsoleLines/HostNewGameLine.h"
	"ConsoleLog/ConsoleLines/EdictUsageLine.cpp"
	"ConsoleLog/ConsoleLines/EdictUsageLine.h"
#	Unused Parser
#	"ConsoleLog/ConsoleLines/CvarlistConvarLine.cpp"
#	"ConsoleLog/ConsoleLines/CvarlistConvarLine.h"
	"ConsoleLog/ConsoleLineListener.cpp"
	"ConsoleLog/ConsoleLineListener.h"
	"ConsoleLog/NetworkStatus.cpp"
	"ConsoleLog/NetworkStatus.h"
	"DB/DBHelpers.h"
	"DB/DBHelpers.cpp"
	"DB/TempDB.h"
	"DB/TempDB.cpp"
	"GameData/MatchmakingQueue.h"
	"GameData/TFClassType.h"
	"GameData/TFParty.h"
	"GameData/UserMessageType.h"
	"Networking/GithubAPI.h"
	"Networking/GithubAPI.cpp"
	"Networking/HTTPClient.h"
	"Networking/HTTPClient.cpp"
	"Networking/HTTPHelpers.h"
	"Networking/HTTPHelpers.cpp"
	"Networking/LogsTFAPI.cpp"
	"Networking/LogsTFAPI.h"
	"Networking/NetworkHelpers.h"
	"Networking/NetworkHelpers.cpp"
	"Networking/SteamAPI.h"
	"Networking/SteamAPI.cpp"
	"Networking/SteamHistoryAPI.h"
	"Networking/SteamHistoryAPI.cpp"
	"Platform/Platform.h"
	"SetupFlow/AddonManagerPage.h"
	"SetupFlow/AddonManagerPage.cpp"
	"SetupFlow/BasicSettingsPage.h"
	"SetupFlow/BasicSettingsPage.cpp"
	"SetupFlow/ChatWrappersGeneratorPage.h"
	"SetupFlow/ChatWrappersGeneratorPage.cpp"
	"SetupFlow/ChatWrappersVerifyPage.h"
	"SetupFlow/ChatWrappersVerifyPage.cpp"
	"SetupFlow/CheckSteamOpenPage.cpp"
	"SetupFlow/ISetupFlowPage.h"
	"SetupFlow/NetworkSettingsPage.h"
	"SetupFlow/NetworkSettingsPage.cpp"
	"SetupFlow/PermissionsCheckPage.cpp"
	"SetupFlow/SetupFlow.cpp"
	"SetupFlow/SetupFlow.h"
	"SetupFlow/TF2CommandLinePage.h"
	"SetupFlow/TF2CommandLinePage.cpp"
	"SetupFlow/UpdateCheckPage.cpp"
	"UI/ImGui_TF2BotDetector.cpp"
	"UI/ImGui_TF2BotDetector.h"
	"UI/MainWindow.h"
	"UI/MainWindow.cpp"
	"UI/MainWindow.Scoreboard.cpp"
	"UI/SettingsWindow.cpp"
	"UI/SettingsWindow.h"
	"UI/PlayerListManagementWindow.cpp"
	"UI/PlayerListManagementWindow.h"
	"Util/JSONUtils.h"
	"Util/PathUtils.cpp"
	"Util/PathUtils.h"
	"Util/TextUtils.cpp"
	"Util/TextUtils.h"
	"Util/ImguiHelpers.h"
	"Util/ScopeGuards.h"
	"Util/ScopeGuards.cpp"
	"Util/StorageHelper.h"
	"Application.cpp"
	"Application.h"
	"BaseTextures.h"
	"BaseTextures.cpp"
	"BatchedAction.h"
	"Bitmap.h"
	"Bitmap.cpp"
	"Clock.cpp"
	"Clock.h"
	"CompensatedTS.cpp"
	"CompensatedTS.h"
	"Config/ChatWrappers.cpp"
	"Config/ChatWrappers.h"
	"DLLMain.cpp"
	"DLLMain.h"
	"Filesystem.cpp"
	"Filesystem.h"
	"GenericErrors.cpp"
	"GenericErrors.h"
	"GlobalDispatcher.h"
	"GameData/IPlayer.cpp"
	"GameData/IPlayer.h"
	"GameData/Player.h"
	"GameData/Player.cpp"
	"Log.cpp"
	"Log.h"
	"ModeratorLogic.cpp"
	"ModeratorLogic.h"
	"PlayerStatus.h"
	"SteamID.cpp"
	"SteamID.h"
	"TextureManager.h"
	"TextureManager.cpp"
	"GameData/TFConstants.h"
	"UpdateManager.h"
	"UpdateManager.cpp"
	"Version.h"
	"Version.cpp"
	"WorldEventListener.cpp"
	"WorldEventListener.h"
	"WorldState.cpp"
	"WorldState.h"
)

target_precompile_headers(tf2_bot_detector
	PUBLIC
		"Clock.h"
	PRIVATE
		"Log.h"
)

if(WIN32)
	configure_file(Resources.base.rc Resources.rc)

	target_sources(tf2_bot_detector PRIVATE
		"Platform/Windows/Processes.cpp"
		"Platform/Windows/Shell.cpp"
		"Platform/Windows/Steam.cpp"
		"Platform/Windows/WindowsHelpers.h"
		"Resources.rc"
		"Platform/Windows/Windows.cpp"
		"Platform/Windows/PlatformInstall.cpp"
		"Platform/Windows/Platform.cpp"
	)
else()
	target_sources(tf2_bot_detector PRIVATE
		"Platform/Linux/Processes.cpp"
		"Platform/Linux/Shell.cpp"
		"Platform/Linux/Steam.cpp"
		"Platform/Linux/LinuxHelpers.h"
		"Platform/Linux/PlatformInstall.cpp"
		"Platform/Linux/Platform.cpp"
	)
endif()

if (TF2BD_ENABLE_DISCORD_INTEGRATION)
	target_compile_definitions(tf2_bot_detector PRIVATE TF2BD_ENABLE_DISCORD_INTEGRATION)
	find_library(DISCORD_GAME_SDK discord_game_sdk) 
	message("DISCORD_GAME_SDK = ${DISCORD_GAME_SDK}")

	# (linux) try to find it as discord_game_sdk.so 
	if (NOT DISCORD_GAME_SDK)
		message("-- Retrying DISCORD_GAME_SDK as discord_game_sdk.so")
		find_library(DISCORD_GAME_SDK discord_game_sdk.so) 
		message("DISCORD_GAME_SDK = ${DISCORD_GAME_SDK}")
	endif()

	target_link_libraries(tf2_bot_detector PRIVATE ${DISCORD_GAME_SDK})

	find_path(GAME_SDK_INCLUDE discord-game-sdk/discord.h)
	target_include_directories(tf2_bot_detector PRIVATE ${DISCORD_GAME_SDK_INCLUDE})
	message("DISCORD_GAME_SDK_INCLUDE = ${DISCORD_GAME_SDK_INCLUDE}")

	find_library(DISCORD_CPP_GAME_SDK discord_game_sdk_cpp)

	# (linux) discord_game_sdk_cpp <- "_cpp" why is that needed in windows target
	# i will never know
	if (NOT DISCORD_CPP_GAME_SDK)
		find_library(DISCORD_CPP_GAME_SDK discord_game_sdk)
	endif()

	message("DISCORD_CPP_GAME_SDK = ${DISCORD_CPP_GAME_SDK}")

	# (linux) try to find it as discord_game_sdk.so 
	if (NOT DISCORD_CPP_GAME_SDK)
		message("-- Retrying DISCORD_CPP_GAME_SDK as discord_game_sdk.so")
		find_library(DISCORD_CPP_GAME_SDK discord_game_sdk.so) 
		message("DISCORD_CPP_GAME_SDK = ${DISCORD_CPP_GAME_SDK}")
		message("NOTE: this may not build! try renaming your discord_game_sdk.so to libdiscord_game_sdk.so!")
	endif()

	# lol.
	if(LINUX)
		target_sources(tf2_bot_detector PRIVATE
			"${DISCORD_GAME_SDK_INCLUDE}/voice_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/user_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/types.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/store_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/storage_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/relationship_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/overlay_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/network_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/lobby_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/image_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/application_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/activity_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/achievement_manager.cpp"
			"${DISCORD_GAME_SDK_INCLUDE}/core.cpp"
		)
	endif()

	target_link_libraries(tf2_bot_detector PRIVATE ${DISCORD_CPP_GAME_SDK})

	target_sources(tf2_bot_detector PRIVATE
		"DiscordRichPresence.cpp"
		"DiscordRichPresence.h"
	)

	# We really only need this library for one thing in the DRP integration.
	# Later on it will probably be moved out of this if statement when it is required elsewhere in the code.
	find_package(cryptopp CONFIG REQUIRED)
	target_link_libraries(tf2_bot_detector PRIVATE cryptopp-static)
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(libzippp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(cpprestsdk CONFIG REQUIRED)

target_link_libraries(tf2_bot_detector PRIVATE
	tf2_bot_detector::common
	tf2_bot_detector_renderer
	ValveFileVDF
	libzip::zip
	libzippp::libzippp
	SourceRCON
	nlohmann_json::nlohmann_json
	fmt::fmt
	BZip2::BZip2
	SQLiteCpp
	ZLIB::ZLIB
	cpprestsdk::cpprest
)

if (TF2BD_ENABLE_TESTS)
	enable_testing()

	find_package(Catch2 CONFIG REQUIRED)
	target_link_libraries(tf2_bot_detector PRIVATE Catch2::Catch2)
	target_compile_definitions(tf2_bot_detector PRIVATE TF2BD_ENABLE_TESTS)
	target_sources(tf2_bot_detector PRIVATE
		"Tests/Catch2.cpp"
		"Tests/ConsoleLineTests.cpp"
		"Tests/FormattingTests.cpp"
		"Tests/HumanDurationTests.cpp"
		"Tests/PlayerRuleTests.cpp"
		"Tests/Tests.h"
	)

	SET(TF2BD_ENABLE_CLI_EXE true)

	add_test(NAME TF2BD_Tests COMMAND tf2_bot_detector_cli --run-tests
		WORKING_DIRECTORY staging
	)
endif()

if(TF2BD_ENABLE_CLI_EXE)
	add_executable(tf2_bot_detector_cli "Launcher/main.cpp")
	target_include_directories(tf2_bot_detector_cli PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
	target_link_libraries(tf2_bot_detector_cli PRIVATE tf2_bot_detector)
	target_compile_features(tf2_bot_detector_cli PUBLIC cxx_std_17)
endif()
