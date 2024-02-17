#include "../Platform.h"
#include "Util/TextUtils.h"
#include "Log.h"

#include <mh/coroutine/future.hpp>
#include <mh/memory/cached_variable.hpp>
#include <mh/error/ensure.hpp>
#include <mh/text/case_insensitive_string.hpp>
#include <mh/text/codecvt.hpp>
#include <mh/text/formatters/error_code.hpp>
#include <mh/text/insertion_conversion.hpp>
#include <mh/text/string_insertion.hpp>

#include <atomic>
#include <cassert>
#include <chrono>
#include <exception>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <fstream>

#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include "LinuxHelpers.h"

// TODO: 64bit binaries
bool tf2_bot_detector::Processes::IsTF2Running()
{
	static mh::cached_variable hl2_exe(std::chrono::seconds(2), []() { return IsProcessRunning("hl2_linux"); });
	static mh::cached_variable tf64_exe(std::chrono::seconds(2), []() { return IsProcessRunning("tf_linux64"); });   
	return tf64_exe.get() || hl2_exe.get();
}

bool tf2_bot_detector::Processes::IsSteamRunning()
{
	static mh::cached_variable steam_pid(std::chrono::seconds(2), []() { return Linux::GetSteamPID(); });
    static mh::cached_variable steam_running(std::chrono::seconds(1), []() { return Linux::IsProcessRunningPid(steam_pid.get()); });
	return steam_running.get();
}

bool tf2_bot_detector::Processes::IsProcessRunning(const std::string_view& processName)
{
    pid_t pid; 
    
    if (Linux::processPids.contains(processName)) {
        pid = Linux::processPids.at(processName);
    }
    else {
        pid = Linux::getPidFromProcessName(std::string(processName));
        Linux::processPids.insert(std::pair<std::string_view, pid_t>(processName, pid));
    }

    bool value = Linux::IsProcessRunningPid(pid);

    if (!value) {
        Linux::processPids.erase(processName);
    }

    return value;
}

// deprecated and unused function it seems, so not even gonna bother implementing atm
void tf2_bot_detector::Processes::RequireTF2NotRunning() {}

void tf2_bot_detector::Processes::Launch(const std::filesystem::path& executable,
	const std::vector<std::string>& args, bool elevated)
{
	std::string cmdLine;

	for (const auto& arg : args)
		cmdLine << std::quoted(arg) << ' ';

	return Launch(executable, cmdLine, elevated);
}

void tf2_bot_detector::Processes::Launch(const std::filesystem::path& executable,
	const std::string_view& args, bool elevated)
{
    std::string execute_command;

    // TODO: implement elevated

    execute_command = fmt::format("{} {}", executable, args);
    
    system(execute_command.c_str());
}

int tf2_bot_detector::Processes::GetCurrentProcessID()
{
	return ::getpid();
}

// TODO: implement (low priority)
size_t tf2_bot_detector::Processes::GetCurrentRAMUsage()
{
    return 1;
}

mh::task<std::vector<std::string>> tf2_bot_detector::Processes::GetTF2CommandLineArgsAsync()
{
    std::vector<std::string> temp = {
        "-usercon +developer 1 +contimes 0 +ip 0.0.0.0 +sv_rcon_whitelist_address 127.0.0.1 +sv_quota_stringcmdspersecond 1000000 +rcon_password asdf +exec autoexec.cfg +hostport 12345 +net_start +con_timestamp 1 -condebug -conclearlog -novid"
    };

    // this doesn't have to be a task cuz we just get it from /proc
    // just make a ready task 
    return mh::make_ready_task<std::vector<std::string>>(std::move(temp));
}
