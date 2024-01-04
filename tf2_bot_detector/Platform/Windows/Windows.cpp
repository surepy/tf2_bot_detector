#include <ntstatus.h>
#define WIN32_NO_STATUS

#include "Platform/Platform.h"
#include "Platform/PlatformCommon.h"
#include "Util/TextUtils.h"
#include "Log.h"
#include "WindowsHelpers.h"

#include <mh/error/ensure.hpp>
#include <mh/error/exception_details.hpp>
#include <mh/text/codecvt.hpp>
#include <mh/text/format.hpp>
#include <mh/text/formatters/error_code.hpp>
#include <mh/text/stringops.hpp>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Shlobj.h>
#include <winternl.h>

using namespace tf2_bot_detector;
using namespace std::string_view_literals;

static std::filesystem::path GetKnownFolderPath(const KNOWNFOLDERID& id)
{
	PWSTR str;
	CHECK_HR(SHGetKnownFolderPath(id, 0, nullptr, &str));

	std::filesystem::path retVal(str);

	CoTaskMemFree(str);
	return retVal;
}

static bool IsReallyWindows10OrGreater()
{
	using RtlGetVersionFn = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW lpVersionInformation);

	static const auto s_RtlGetVersionFn = reinterpret_cast<RtlGetVersionFn>(
		tf2_bot_detector::Platform::GetProcAddressHelper("ntdll.dll", "RtlGetVersion", true));

	RTL_OSVERSIONINFOW info{};
	info.dwOSVersionInfoSize = sizeof(info);
	const auto result = s_RtlGetVersionFn(&info);
	assert(result == STATUS_SUCCESS);

	return info.dwMajorVersion >= 10;
}

#pragma warning(disable: 6262)
std::filesystem::path tf2_bot_detector::Platform::GetCurrentExeDir()
{
	// C6262: Function uses '65544' bytes of stack. Consider moving some data to heap.
	// the maximum possible directory length in windows, apparently.
	WCHAR path[32768];
	const auto length = GetModuleFileNameW(nullptr, path, (DWORD)std::size(path));

	const auto error = GetLastError();
	if (error != ERROR_SUCCESS)
		throw tf2_bot_detector::Windows::GetLastErrorException(E_FAIL, error, "Call to GetModuleFileNameW() failed");

	if (length == 0)
		throw std::runtime_error("Call to GetModuleFileNameW() failed: return value was 0");

	return std::filesystem::path(path, path + length).parent_path();
}
#pragma warning(default: 6262)

// unused, remove?
std::filesystem::path tf2_bot_detector::Platform::GetLegacyAppDataDir()
{
	/*
	if (auto winrt = GetWinRTInterface())
	{
		try
		{
			auto packageFamilyName = GetWinRTInterface()->GetCurrentPackageFamilyName();
			if (packageFamilyName.empty())
				return {};

			return GetKnownFolderPath(FOLDERID_LocalAppData) / "Packages" / packageFamilyName / "LocalCache" / "Roaming";
		}
		catch (...)
		{
			LogException();
		}
	}
	*/

	return {};
}

std::filesystem::path tf2_bot_detector::Platform::GetRootLocalAppDataDir()
{
	return GetKnownFolderPath(FOLDERID_LocalAppData);
}

std::filesystem::path tf2_bot_detector::Platform::GetRootRoamingAppDataDir()
{
	return GetKnownFolderPath(FOLDERID_RoamingAppData);
}

std::filesystem::path tf2_bot_detector::Platform::GetRootTempDataDir()
{
	return std::filesystem::temp_directory_path();
}
