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

// PID cache
static std::unordered_map<std::string_view, pid_t> processPids;

// chatgpt generated code cuz lazy
pid_t getPidFromProcessName(const std::string& processName) {
    DIR* dir = opendir("/proc");
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Check if the entry is a directory and its name is a number (PID)
            if (entry->d_type == DT_DIR && std::isdigit(entry->d_name[0])) {
                // Open the cmdline file to read the process name
                std::filesystem::path cmdlinePath = std::filesystem::current_path().root_directory() / "proc" / entry->d_name / "cmdline";
                FILE* cmdlineFile = fopen(cmdlinePath.c_str(), "r");
                if (cmdlineFile != nullptr) {
                    char cmdline[1024];
                    fgets(cmdline, sizeof(cmdline), cmdlineFile);
                    fclose(cmdlineFile);
                    
                    // Compare the process name with the given name
                    if (strstr(cmdline, processName.c_str()) != nullptr) {
                        closedir(dir);
                        return atoi(entry->d_name); // Convert directory name to PID
                    }
                }
            }
        }
        closedir(dir);
    }
    return -1; // Return -1 if the process name is not found
}

// if you dont have ~/.steam/steam.pid, this will fail
// but i dont really understand why you won't have .steam/steam.pid
pid_t GetSteamPID () {
    std::filesystem::path steam_pid_path = std::filesystem::path(getenv("HOME")) / ".steam" / "steam.pid";
    std::ifstream steam_pid_file(steam_pid_path);

    pid_t ret; 
    steam_pid_file >> ret;
    return ret;
}

bool IsProcessRunningPid(pid_t pid) {
    if (pid == -1) {
        return false;
    }
    return kill(pid, 0) == 0;
}

// TODO: 64bit binaries
bool tf2_bot_detector::Processes::IsTF2Running()
{
	static mh::cached_variable hl2_exe(std::chrono::seconds(2), []() { return IsProcessRunning("hl2_linux"); });
	return hl2_exe.get();
}

bool tf2_bot_detector::Processes::IsSteamRunning()
{
	static mh::cached_variable steam_pid(std::chrono::seconds(2), []() { return GetSteamPID(); });
    static mh::cached_variable steam_running(std::chrono::seconds(1), []() { return IsProcessRunningPid(steam_pid.get()); });
	return steam_running.get();
}

bool tf2_bot_detector::Processes::IsProcessRunning(const std::string_view& processName)
{
    pid_t pid; 
    
    if (processPids.contains(processName)) {
        pid = processPids.at(processName);
    }
    else {
        pid = getPidFromProcessName(std::string(processName));
        processPids.insert(std::pair<std::string_view, pid_t>(processName, pid));
    }

    bool value = IsProcessRunningPid(pid);

    if (!value) {
        processPids.erase(processName);
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
    // this doesn't have to be a task cuz we just get it from /proc
    // just make a ready task 
    return mh::make_ready_task<std::vector<std::string>>();
}
