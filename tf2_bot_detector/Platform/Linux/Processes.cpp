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

#include <Windows.h>
#include <shellapi.h>
#include "WindowsHelpers.h"
#include <comutil.h>
#include <Wbemidl.h>
#include <TlHelp32.h>
#include <wrl/client.h>
#include <Psapi.h>
