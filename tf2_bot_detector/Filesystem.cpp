#include "Filesystem.h"
#include "Log.h"
#include "Platform/Platform.h"

#include <mh/concurrency/thread_sentinel.hpp>
#include <mh/text/format.hpp>
#include <mh/text/string_insertion.hpp>
#include <mh/utility.hpp>

#include <fstream>

using namespace tf2_bot_detector;

namespace
{
	class Filesystem final : public IFilesystem
	{
	public:
		void Init() override;

		mh::generator<std::filesystem::path> GetSearchPaths() const override;

		std::filesystem::path ResolvePath(const std::filesystem::path& path, PathUsage usage) const override;
		std::string ReadFile(std::filesystem::path path) const override;
		void WriteFile(std::filesystem::path path, const void* begin, const void* end, PathUsage usage) const override;

		std::filesystem::path GetLocalAppDataDir() const override;
		std::filesystem::path GetRoamingAppDataDir() const override;
		std::filesystem::path GetTempDir() const override;

		mh::generator<std::filesystem::directory_entry> IterateDir(std::filesystem::path path, bool recursive,
			std::filesystem::directory_options options) const override;

	private:
		bool m_IsInit = false;
		void EnsureInit(MH_SOURCE_LOCATION_AUTO(location)) const;

		static constexpr char NON_PORTABLE_MARKER[] = ".non_portable";
		static constexpr char APPDATA_SUBFOLDER[] = "TF2 Bot Detector";
		std::vector<std::filesystem::path> m_SearchPaths;

		mh::thread_sentinel m_Sentinel;

		std::filesystem::path m_ExeDir;
		std::filesystem::path m_WorkingDir;
#ifndef __linux__
		std::filesystem::path m_LocalAppDataDir;
		std::filesystem::path m_RoamingAppDataDir;
#endif
		//std::filesystem::path m_MutableDataDir = ChooseMutableDataPath();
	};
}

IFilesystem& IFilesystem::Get()
{
	static Filesystem s_Filesystem;
	return s_Filesystem;
}

void Filesystem::Init()
{
	m_Sentinel.check();

	if (!m_IsInit)
	{
		m_IsInit = true;
		try
		{
			DebugLog("Initializing filesystem...");

			m_ExeDir = Platform::GetCurrentExeDir();
#ifndef __linux__
			m_LocalAppDataDir = Platform::GetRootLocalAppDataDir() / APPDATA_SUBFOLDER;
			m_RoamingAppDataDir = Platform::GetRootRoamingAppDataDir() / APPDATA_SUBFOLDER;
#endif
			m_SearchPaths.insert(m_SearchPaths.begin(), m_ExeDir);

			m_WorkingDir = std::filesystem::current_path();

			// support for launching through steam.
			if (m_WorkingDir != m_ExeDir) {

				DebugLog("m_ExeDir differs from working directory!", m_ExeDir);

				// create cfg inside Team Fortress 2 folder (cuz why not)
				if (std::filesystem::create_directories("cfg"))
					DebugLog("Created {}/cfg", m_ExeDir);

				// NOTE: this creates a kind of nasty unintended behaviors where if a setting.json or something is in that directory,
				// it will have a higher priority and just wipe the other one.
				// don't ask me how I know
				m_SearchPaths.insert(m_SearchPaths.begin(), m_WorkingDir);
			}

			// we've launched somewhere that differs from PATH, we should probably change back to our exedir cuz that's where we work.
			//  || !(m_WorkingDir / "hl2.exe").empty() || !(m_WorkingDir / "hl2_linux").empty()
#ifdef _WIN32
			if ((m_WorkingDir / "tf2_bot_detector.dll").empty()) 
#else
			if ((m_WorkingDir / "tf2_bot_detector").empty()) 
#endif
			{
				DebugLog("We can't find tf2_bot_detector.dll in m_WorkingDir!", m_ExeDir);
				// If we crash, we want our working directory to be somewhere we can write to.
				std::filesystem::current_path(m_ExeDir);
				DebugLog("Set working directory to {}", m_ExeDir);
			}

			{
				std::string initMsg = "Filesystem initialized. Search paths:";
				for (const auto& searchPath : m_SearchPaths)
					initMsg << "\n\t" << searchPath;

				DebugLog(std::move(initMsg));
			}

			DebugLog("\tWorkingDir: {}", m_WorkingDir);
			DebugLog("\tLocalAppDataDir: {}", GetLocalAppDataDir());
			DebugLog("\tRoamingAppDataDir: {}", GetRoamingAppDataDir());
			DebugLog("\tTempDir: {}", GetTempDir());
			std::filesystem::create_directories(GetTempDir());
		}
		catch (...)
		{
			LogFatalException("Failed to initialize filesystem");
		}
	}
}

mh::generator<std::filesystem::path> Filesystem::GetSearchPaths() const
{
	for (auto searchPath : m_SearchPaths)
		co_yield searchPath;
}

std::filesystem::path Filesystem::ResolvePath(const std::filesystem::path& path, PathUsage usage) const
{
	if (path.is_absolute())
		return path;

	EnsureInit();

	auto retVal = std::invoke([&]() -> std::filesystem::path
	{
		if (usage == PathUsage::Read)
		{
			std::vector<std::filesystem::path> fullSearchPaths;

			for (const auto& searchPath : m_SearchPaths)
			{
				auto fullPath = searchPath / path;
				fullSearchPaths.push_back(fullPath);
				if (std::filesystem::exists(fullPath))
				{
					assert(fullPath.is_absolute());
					return fullPath;
				}
			}

			std::string debugMsg = mh::format("Unable to find {} in any search path. Full search paths [{} paths]:",
				path, fullSearchPaths.size());

			for (const std::filesystem::path& fsp : fullSearchPaths)
				debugMsg << "\n\t" << fsp;

			DebugLogWarning(debugMsg);
			return {};
		}
		else if (usage == PathUsage::WriteLocal)
		{
			return GetLocalAppDataDir() / path;
		}
		else if (usage == PathUsage::WriteRoaming)
		{
			return GetRoamingAppDataDir() / path;
		}
		else
		{
			throw std::invalid_argument(mh::format("{}: Unknown PathUsage value {}", __FUNCTION__, int(usage)));
		}
	});

	DebugLog("ResolvePath({}, {}) -> {}", path, mh::enum_fmt(usage), retVal);
	return std::move(retVal);
}

std::string Filesystem::ReadFile(std::filesystem::path path) const try
{
	path = ResolvePath(path, PathUsage::Read);
	if (path.empty())
		throw std::filesystem::filesystem_error("ResolvePath returned an empty path.", path, make_error_code(std::errc::no_such_file_or_directory));

	std::ifstream file;
	file.exceptions(std::ios::badbit | std::ios::failbit);
	file.open(path, std::ios::binary);

	file.seekg(0, std::ios::end);

	const auto length = file.tellg();
	file.seekg(0, std::ios::beg);

	std::string retVal;
	retVal.resize(size_t(length));
	file.read(retVal.data(), length);

	return retVal;
}
catch (...)
{
	DebugLogException("Filename: {}", path);
	throw;
}

void Filesystem::WriteFile(std::filesystem::path path, const void* begin, const void* end, PathUsage usage) const try
{
	path = ResolvePath(path, usage);

	// Create any missing directories
	if (auto folderPath = mh::copy(path).remove_filename(); std::filesystem::create_directories(folderPath))
		DebugLog("Created one or more directories in the path {}", folderPath);

	std::ofstream file;
	file.exceptions(std::ios::badbit | std::ios::failbit);
	file.open(path, std::ios::binary | std::ios::trunc);

	const auto bytes = uintptr_t(end) - uintptr_t(begin);
	file.write(reinterpret_cast<const char*>(begin), bytes);
}
catch (...)
{
	LogException("Filename: {}", path);
	throw;
}

std::filesystem::path Filesystem::GetLocalAppDataDir() const
{
	EnsureInit();

	return m_ExeDir;
}

std::filesystem::path Filesystem::GetRoamingAppDataDir() const
{
	EnsureInit();

	return m_ExeDir;
}

std::filesystem::path Filesystem::GetTempDir() const
{
	EnsureInit();

	return m_ExeDir / "temp";
}

void Filesystem::EnsureInit(const mh::source_location& location) const
{
	if (!m_IsInit)
	{
		assert(false);
		LogFatalError(location, "Filesystem::EnsureInit failed");
	}
}

template<typename TIter>
static mh::generator<std::filesystem::directory_entry> IterateDirImpl(
	const std::vector<std::filesystem::path>& searchPaths, std::filesystem::path path, std::filesystem::directory_options options)
{
	for (std::filesystem::path searchPath : searchPaths)
	{
		searchPath /= path;

		if (std::filesystem::exists(searchPath))
		{
			for (const std::filesystem::directory_entry& entry : TIter(searchPath, options))
				co_yield entry;
		}
	}
}

mh::generator<std::filesystem::directory_entry> Filesystem::IterateDir(std::filesystem::path path, bool recursive,
	std::filesystem::directory_options options) const
{
	assert(!path.is_absolute());

	if (recursive)
		return IterateDirImpl<std::filesystem::recursive_directory_iterator>(m_SearchPaths, std::move(path), options);
	else
		return IterateDirImpl<std::filesystem::directory_iterator>(m_SearchPaths, std::move(path), options);
}
