#pragma once

#include <mh/coroutine/generator.hpp>
#include <mh/reflection/enum.hpp>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace tf2_bot_detector
{
	enum class PathUsage
	{
		Read,

		WriteRoaming,
		WriteLocal,
		Write [[deprecated]] = WriteRoaming,
	};

	class IFilesystem
	{
	public:
		virtual ~IFilesystem() = default;

		static IFilesystem& Get();

		virtual void Init() = 0;

		virtual mh::generator<std::filesystem::path> GetSearchPaths() const = 0;

		virtual std::filesystem::path ResolvePath(const std::filesystem::path& path, PathUsage usage) const = 0;

		virtual std::filesystem::path GetLocalAppDataDir() const = 0;
		virtual std::filesystem::path GetTempDir() const = 0;

		//virtual std::fstream OpenFile(const std::filesystem::path& path) = 0;
		virtual std::string ReadFile(std::filesystem::path path) const = 0;
		virtual void WriteFile(std::filesystem::path path, const void* begin, const void* end, PathUsage usage) const = 0;

		virtual mh::generator<std::filesystem::directory_entry> IterateDir(std::filesystem::path path, bool recursive,
			std::filesystem::directory_options options = std::filesystem::directory_options::none) const = 0;

		void WriteFile(const std::filesystem::path& path, const std::string_view& data, PathUsage usage) const
		{
			return WriteFile(path, data.data(), data.data() + data.size(), usage);
		}

		static std::filesystem::path GetLogsDir(const std::filesystem::path& baseDataDir)
		{
			return baseDataDir / "logs";
		}
		std::filesystem::path GetLogsDir() const { return GetLogsDir(GetLocalAppDataDir()); }

		static std::filesystem::path GetConfigDir(const std::filesystem::path& baseDataDir)
		{
			return baseDataDir / "cfg";
		}
		std::filesystem::path GetConfigDir() const { return GetConfigDir(GetLocalAppDataDir()); }

		bool Exists(const std::filesystem::path& path) const
		{
			return !ResolvePath(path, PathUsage::Read).empty();
		}
	};
}

MH_ENUM_REFLECT_BEGIN(tf2_bot_detector::PathUsage)
	MH_ENUM_REFLECT_VALUE(Read)
	MH_ENUM_REFLECT_VALUE(WriteRoaming)
	MH_ENUM_REFLECT_VALUE(WriteLocal)
MH_ENUM_REFLECT_END()
