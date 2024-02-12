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

#include <unistd.h>
#include <dirent.h>
#include <signal.h>

// chatgpt generated code cuz lazy
// TODO: probably cache all the pids we need.
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

// TODO: 64bit binaries
bool tf2_bot_detector::Processes::IsTF2Running()
{
	static mh::cached_variable hl2_exe(std::chrono::seconds(5), []() { return IsProcessRunning("hl2_linux"); });
	return hl2_exe.get();

}

bool tf2_bot_detector::Processes::IsSteamRunning()
{
    // usually located at ~/.local/shrea/Steam/steam.sh
	static mh::cached_variable m_CachedValue(std::chrono::seconds(1), []() { return IsProcessRunning("steam.sh"); });
	return m_CachedValue.get();
}

bool tf2_bot_detector::Processes::IsProcessRunning(const std::string_view& processName)
{
    // lmfao
    pid_t pid = getPidFromProcessName(std::string(processName));
    if (pid == -1) {
        return false;
    }
    return kill(pid, 0) == 0;
}

// deprecated and unused function it seems
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
    
}

int tf2_bot_detector::Processes::GetCurrentProcessID()
{
	return ::getpid();
}

// TODO: implement
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
