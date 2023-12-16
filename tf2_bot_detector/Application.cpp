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

#include "ConsoleLog/ConsoleLines/LobbyChangedLine.h"
#include "ConsoleLog/ConsoleLines/EdictUsageLine.h"

#include <ScopeGuards.h>
#include <libzippp/libzippp.h>
#include <mh/math/interpolation.hpp>
#include <mh/text/case_insensitive_string.hpp>
#include <mh/text/formatters/error_code.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/text/string_insertion.hpp>
#include <mh/text/stringops.hpp>
#include <srcon/async_client.h>

#include <mh/error/ensure.hpp>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>

using namespace tf2_bot_detector;
// 
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

static TF2BDApplication* s_Application;

namespace tf2_bot_detector
{
	mh::dispatcher& GetDispatcher()
	{
		static mh::dispatcher s_Dispatcher;
		return s_Dispatcher;
	}
}

TF2BDApplication::TF2BDApplication() :
	m_WorldState(IWorldState::Create(m_Settings)),
	m_ActionManager(IRCONActionManager::Create(m_Settings, GetWorld())),
	m_UpdateManager(IUpdateManager::Create(m_Settings))
{
	assert(!s_Application);
	s_Application = this;

	m_TempDB = DB::ITempDB::Create();

	// moved from mainwindow
	ILogManager::GetInstance().CleanupLogFiles();

	GetWorld().AddConsoleLineListener(this);
	GetWorld().AddWorldEventListener(this);

	m_OpenTime = clock_t::now();

	GetActionManager().AddPeriodicActionGenerator<StatusUpdateActionGenerator>();
	GetActionManager().AddPeriodicActionGenerator<ConfigActionGenerator>();
	GetActionManager().AddPeriodicActionGenerator<LobbyDebugActionGenerator>();

	// always run at first tick.
	QueueUpdate();
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

void TF2BDApplication::OnConsoleLineParsed(IWorldState& world, IConsoleLine& parsed)
{
	m_ParsedLineCount++;

	if (parsed.ShouldPrint() && m_MainState)
	{
		while (m_MainState->m_PrintingLines.size() > m_MainState->MAX_PRINTING_LINES)
			m_MainState->m_PrintingLines.pop_back();

		m_MainState->m_PrintingLines.push_front(parsed.shared_from_this());
	}

	switch (parsed.GetType())
	{
	case ConsoleLineType::LobbyChanged:
	{
		auto& lobbyChangedLine = static_cast<const LobbyChangedLine&>(parsed);
		const LobbyChangeType changeType = lobbyChangedLine.GetChangeType();

		if (changeType == LobbyChangeType::Created || changeType == LobbyChangeType::Updated)
			GetActionManager().QueueAction<LobbyUpdateAction>();

		break;
	}
	case ConsoleLineType::EdictUsage:
	{
		auto& usageLine = static_cast<const EdictUsageLine&>(parsed);
		m_EdictUsageSamples.push_back({ usageLine.GetTimestamp(), usageLine.GetUsedEdicts(), usageLine.GetTotalEdicts() });

		while (m_EdictUsageSamples.front().m_Timestamp < (usageLine.GetTimestamp() - 5min))
			m_EdictUsageSamples.erase(m_EdictUsageSamples.begin());

		break;
	}

	default: break;
	}
}

void TF2BDApplication::OnConsoleLineUnparsed(IWorldState& world, const std::string_view& text)
{
	// why does it add when it's called unparsed wtf
	m_ParsedLineCount++;
}

void TF2BDApplication::OnConsoleLogChunkParsed(IWorldState& world, bool consoleLinesUpdated)
{
	assert(&world == &GetWorld());

	if (consoleLinesUpdated)
		UpdateServerPing(GetCurrentTimestampCompensated());
}

bool TF2BDApplication::IsTimeEven() const
{
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(clock_t::now() - m_OpenTime);
	return !(seconds.count() % 2);
}

float TF2BDApplication::TimeSine(float interval, float min, float max) const
{
	const auto elapsed = (clock_t::now() - m_OpenTime) % std::chrono::duration_cast<clock_t::duration>(std::chrono::duration<float>(interval));
	const auto progress = std::chrono::duration<float>(elapsed).count() / interval;
	return mh::remap(std::sin(progress * 6.28318530717958647693f), -1.0f, 1.0f, min, max);
}

time_point_t TF2BDApplication::GetCurrentTimestampCompensated() const
{
	return GetWorld().GetCurrentTime();
}

void TF2BDApplication::UpdateServerPing(time_point_t timestamp)
{
	if ((timestamp - m_LastServerPingSample) <= 7s)
		return;

	float totalPing = 0;
	uint16_t samples = 0;

	for (IPlayer& player : GetWorld().GetPlayers())
	{
		if (player.GetLastStatusUpdateTime() < (timestamp - 20s))
			continue;

		auto& data = player.GetOrCreateData<PlayerExtraData>(player);
		totalPing += data.GetAveragePing();
		samples++;
	}

	m_ServerPingSamples.push_back({ timestamp, uint16_t(totalPing / samples) });
	m_LastServerPingSample = timestamp;

	while ((timestamp - m_ServerPingSamples.front().m_Timestamp) > 5min)
		m_ServerPingSamples.erase(m_ServerPingSamples.begin());
}

void tf2_bot_detector::TF2BDApplication::Update()
{
	if (m_Paused)
		return;

	GetDispatcher().run_for(10ms);

	GetWorld().Update();
	m_UpdateManager->Update();

	if (m_Settings.m_Unsaved.m_RCONClient)
		m_Settings.m_Unsaved.m_RCONClient->set_logging(m_Settings.m_Logging.m_RCONPackets);

	if (m_SetupFlow.OnUpdate(m_Settings))
	{
		m_MainState.reset();
	}
	else
	{
		if (!m_MainState)
			m_MainState.emplace(*this);

		m_MainState->m_Parser.Update();
		GetModLogic().Update();

		m_MainState->OnUpdateDiscord();
	}

	GetActionManager().Update();

	// set our "should update even tabbed out" variable to false
	if (b_ShouldUpdate) {
		b_ShouldUpdate = false;
	}
}

float TF2BDApplication::PlayerExtraData::GetAveragePing() const
{
	unsigned totalPing = m_Parent->GetPing();
	unsigned samples = 1;

	for (const auto& entry : m_PingHistory)
	{
		totalPing += entry.m_Ping;
		samples++;
	}

	return totalPing / float(samples);
}

TF2BDApplication::PostSetupFlowState::PostSetupFlowState(TF2BDApplication& app) :
	m_Parent(&app),
	m_ModeratorLogic(IModeratorLogic::Create(app.GetWorld(), app.m_Settings, app.GetActionManager())),
	m_Parser(app.GetWorld(), app.m_Settings, app.m_Settings.GetTFDir() / "console.log")
{
#ifdef TF2BD_ENABLE_DISCORD_INTEGRATION
	m_DRPManager = IDRPManager::Create(app.m_Settings, app.GetWorld());
#endif
}

void TF2BDApplication::PostSetupFlowState::OnUpdateDiscord()
{
#ifdef TF2BD_ENABLE_DISCORD_INTEGRATION
	const auto curTime = clock_t::now();
	if (!m_DRPManager && m_Parent->m_Settings.m_Discord.m_EnableRichPresence)
	{
		m_DRPManager = IDRPManager::Create(m_Parent->m_Settings, m_Parent->GetWorld());
	}
	else if (m_DRPManager && !m_Parent->m_Settings.m_Discord.m_EnableRichPresence)
	{
		m_DRPManager.reset();
	}

	if (m_DRPManager)
		m_DRPManager->Update();
#endif
}

mh::generator<IPlayer&> TF2BDApplication::PostSetupFlowState::GeneratePlayerPrintData()
{
	IPlayer* printData[33]{};
	auto begin = std::begin(printData);
	auto end = std::end(printData);
	assert(begin <= end);
	auto& world = m_Parent->m_WorldState;
	assert(static_cast<size_t>(end - begin) >= world->GetApproxLobbyMemberCount());

	std::fill(begin, end, nullptr);

	{
		auto* current = begin;
		for (IPlayer& member : world->GetLobbyMembers())
		{
			*current = &member;
			current++;
		}

		if (current == begin)
		{
			// We seem to have either an empty lobby or we're playing on a community server.
			// Just find the most recent status updates.
			for (IPlayer& playerData : world->GetPlayers())
			{
				if (playerData.GetLastStatusUpdateTime() >= (world->GetLastStatusUpdateTime() - 15s))
				{
					*current = &playerData;
					current++;

					if (current >= end)
						break; // This might happen, but we're not in a lobby so everything has to be approximate
				}
			}
		}

		end = current;
	}

	std::sort(begin, end, [](const IPlayer* lhs, const IPlayer* rhs) -> bool
		{
			assert(lhs);
			assert(rhs);
			//if (!lhs && !rhs)
			//	return false;
			//if (auto result = !!rhs <=> !!lhs; !std::is_eq(result))
			//	return result < 0;

			// Intentionally reversed, we want descending kill order
			if (auto killsResult = rhs->GetScores().m_Kills <=> lhs->GetScores().m_Kills; !std::is_eq(killsResult))
				return std::is_lt(killsResult);

			if (auto deathsResult = lhs->GetScores().m_Deaths <=> rhs->GetScores().m_Deaths; !std::is_eq(deathsResult))
				return std::is_lt(deathsResult);

			// Sort by ascending userid
			{
				auto luid = lhs->GetUserID();
				auto ruid = rhs->GetUserID();
				if (luid && ruid)
				{
					if (auto result = *luid <=> *ruid; !std::is_eq(result))
						return std::is_lt(result);
				}
			}

			return false;
		});

	for (auto it = begin; it != end; ++it)
		co_yield **it;
}


bool TF2BDApplication::IsSleepingEnabled() const
{
	return m_Settings.m_SleepWhenUnfocused;
}

/// <summary>
/// force our program to update, even if sleeping is enabled and our application is sleeping,
/// aka wake it up
/// </summary>
/// <returns></returns>
void TF2BDApplication::QueueUpdate()
{
	b_ShouldUpdate = true;
}

/// <summary>
/// should we run Update()?
/// </summary>
/// <returns></returns>
bool TF2BDApplication::ShouldUpdate()
{
	return !this->IsSleepingEnabled() || b_ShouldUpdate;
}
