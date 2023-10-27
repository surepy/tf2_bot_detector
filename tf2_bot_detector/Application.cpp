#include "Application.h"
#include "DB/TempDB.h"

// pasted without looking
#include "DiscordRichPresence.h"
#include "Networking/GithubAPI.h"
#include "Networking/SteamAPI.h"
#include "ConsoleLog/NetworkStatus.h"
#include "Platform/Platform.h"
#include "Actions/ActionGenerators.h"
#include "BaseTextures.h"
#include "Filesystem.h"
#include "GenericErrors.h"
#include "Log.h"
#include "GameData/IPlayer.h"
#include "ReleaseChannel.h"
#include "TextureManager.h"
#include "UpdateManager.h"
#include "Util/PathUtils.h"
#include "Version.h"
#include "GlobalDispatcher.h"
#include "Networking/HTTPClient.h"
#include "UpdateManager.h"

#include <mh/error/ensure.hpp>

#include <cassert>

using namespace tf2_bot_detector;

static TF2BDApplication* s_Application;

TF2BDApplication::TF2BDApplication()
{
	assert(!s_Application);
	s_Application = this;

	m_TempDB = DB::ITempDB::Create();
}

TF2BDApplication::~TF2BDApplication() = default;

TF2BDApplication& TF2BDApplication::GetApplication()
{
	return *mh_ensure(s_Application);
}

DB::ITempDB& TF2BDApplication::GetTempDB()
{
	return *mh_ensure(m_TempDB.get());
}
