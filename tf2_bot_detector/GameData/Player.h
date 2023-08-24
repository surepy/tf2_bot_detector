#pragma once

#include "Clock.h"
#include "SteamID.h"
#include "GameData/TFConstants.h"
#include "GameData/IPlayer.h"
#include "WorldState.h"

#include <mh/error/expected.hpp>

#include <any>
#include <cstdint>
#include <optional>
#include <ostream>
#include <typeindex>

#include "WorldState.h"
#include "Actions/Actions.h"
#include "Config/Settings.h"
#include "ConsoleLog/ConsoleLineListener.h"
#include "ConsoleLog/ConsoleLogParser.h"
#include "GameData/TFClassType.h"
#include "GameData/UserMessageType.h"
#include "Networking/HTTPHelpers.h"
#include "Networking/SteamAPI.h"
#include "Networking/SteamHistoryAPI.h"
#include "Networking/LogsTFAPI.h"
#include "Util/RegexUtils.h"
#include "Util/TextUtils.h"
#include "BatchedAction.h"
#include "GenericErrors.h"
#include "GameData/IPlayer.h"
#include "GameData/Player.h"
#include "Log.h"
#include "WorldEventListener.h"
#include "Config/AccountAges.h"
#include "GlobalDispatcher.h"
#include "Application.h"
#include "DB/TempDB.h"
#include "PlayerStatus.h"

namespace tf2_bot_detector
{

}
