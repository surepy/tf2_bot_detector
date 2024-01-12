#pragma once
#include "IActionManager.h"
#include "Actions/ActionGenerators.h"
#include "Config/Settings.h"
#include "Actions.h"
#include "Log.h"
#include "WorldEventListener.h"
#include "WorldState.h"

#include <mh/text/insertion_conversion.hpp>
#include <mh/text/string_insertion.hpp>
#include <srcon/async_client.h>

#include <filesystem>
#include <iomanip>
#include <queue>
#include <regex>
#include <unordered_set>

namespace tf2_bot_detector
{
	class Settings;
	class IWorldState;

	class RCONActionManager final : public IActionManager, BaseWorldEventListener
	{
	public:
		RCONActionManager(const Settings& settings, IWorldState& world);
		~RCONActionManager();

		static std::unique_ptr<RCONActionManager> Create(const Settings& settings, IWorldState& world);

		void Update();

		bool QueueAction(std::unique_ptr<IAction>&& action);

		template<typename TAction, typename... TArgs>
		bool QueueAction(TArgs&&... args)
		{
			return QueueAction(std::make_unique<TAction>(std::forward<TArgs>(args)...));
		}

		void AddPeriodicActionGenerator(std::unique_ptr<IPeriodicActionGenerator>&& action);

		template<typename TAction, typename... TArgs>
		void AddPeriodicActionGenerator(TArgs&&... args)
		{
			return AddPeriodicActionGenerator(std::make_unique<TAction>(std::forward<TArgs>(args)...));
		}

	private:
		void OnLocalPlayerInitialized(IWorldState& world, bool initialized) override;

		struct RunningCommand
		{
			time_point_t m_StartTime{};
			std::string m_Command;
			std::shared_future<std::string> m_Future;
		};
		std::queue<RunningCommand> m_RunningCommands;
		void ProcessRunningCommands();
		void ProcessQueuedCommands();

		struct Writer;

		static constexpr duration_t UPDATE_INTERVAL = std::chrono::milliseconds(250);

		IWorldState& m_WorldState;
		const Settings& m_Settings;
		time_point_t m_LastUpdateTime{};
		std::vector<std::unique_ptr<IAction>> m_Actions;
		std::vector<std::unique_ptr<IPeriodicActionGenerator>> m_PeriodicActionGenerators;
		std::map<ActionType, time_point_t> m_LastTriggerTime;

		bool ShouldDiscardCommand(const std::string_view& cmd) const;
		bool m_IsDiscardingServerCommands = true;

	public:
		void clearActions() {
			m_Actions.clear();
		}
	};
}
