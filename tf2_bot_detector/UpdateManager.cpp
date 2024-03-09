#include "UpdateManager.h"
#include "Config/Settings.h"
#include "Networking/GithubAPI.h"
#include "Networking/HTTPClient.h"
#include "Networking/HTTPHelpers.h"
#include "Platform/Platform.h"
#include "Util/JSONUtils.h"
#include "Log.h"
#include "ReleaseChannel.h"
#include "Filesystem.h"

#include <libzippp/libzippp.h>
#include <mh/algorithm/multi_compare.hpp>
#include <mh/future.hpp>
#include <mh/error/exception_details.hpp>
#include <mh/raii/scope_exit.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/types/disable_copy_move.hpp>
#include <mh/utility.hpp>
#include <mh/variant.hpp>
#include <nlohmann/json.hpp>

#include <compare>
#include <fstream>
#include <variant>

#ifdef __linux__
#define __FUNCSIG__ __PRETTY_FUNCTION__
#endif

using namespace std::string_view_literals;
using namespace tf2_bot_detector;

namespace tf2_bot_detector
{
	void to_json(nlohmann::json& j, const Platform::OS& d)
	{
		j = mh::find_enum_value_name(d);
	}
	void from_json(const nlohmann::json& j, Platform::OS& d)
	{
		mh::find_enum_value(j.get<std::string_view>(), d);
	}

	void to_json(nlohmann::json& j, const Platform::Arch& d)
	{
		j = mh::find_enum_value_name(d);
	}
	void from_json(const nlohmann::json& j, Platform::Arch& d)
	{
		mh::find_enum_value(j.get<std::string_view>(), d);
	}

	void to_json(nlohmann::json& j, const BuildInfo& d)
	{
		j =
		{
			{ "version", d.m_Version },
			{ "build_type", d.m_ReleaseChannel },
			{ "github_url", d.m_GitHubURL }
		};
	}
	void from_json(const nlohmann::json& j, BuildInfo& d)
	{
		d.m_Version = j.at("version");
		d.m_ReleaseChannel = j.at("build_type");
		try_get_to_defaulted(j, d.m_GitHubURL, "github_url");
	}
}

namespace
{
	class UpdateManager final : public IUpdateManager
	{
		struct AvailableUpdate final : IAvailableUpdate
		{
			AvailableUpdate(UpdateManager& parent, BuildInfo&& buildInfo);

			UpdateManager& m_Parent;
		};

	public:
		UpdateManager(const Settings& settings);

		void Update() override;
		mh::status_reader<UpdateStatus> GetUpdateStatus() const override { return m_State.GetUpdateStatus(); }
		const AvailableUpdate* GetAvailableUpdate() const override;

		void QueueUpdateCheck() override { m_IsUpdateQueued = true; }

	private:
		const Settings& m_Settings;

		struct BaseExceptionData
		{
			BaseExceptionData(const std::type_info& type, std::string message, const std::exception_ptr& exception);

			const std::type_info& m_Type;
			std::string m_Message;
			std::exception_ptr m_Exception;
		};

		template<typename T>
		struct ExceptionData : BaseExceptionData
		{
			using BaseExceptionData::BaseExceptionData;
		};


		// These need to be std::optional<...> because there is a bug in the MSVC STL
		// https://developercommunity.visualstudio.com/content/problem/60897/c-shared-state-futuresstate-default-constructs-the.html
		using State_t = std::variant<
			std::monostate
		>;
		using UpdateCheckState_t = std::variant<std::future<BuildInfo>, AvailableUpdate>;

		struct StateManager
		{
			UpdateCheckState_t& GetUpdateCheckVariant() { return m_UpdateCheckVariant; }
			const UpdateCheckState_t& GetUpdateCheckVariant() const { return m_UpdateCheckVariant; }

			template<typename T, typename... TArgs>
			auto Emplace(const mh::source_location& location, UpdateStatus status, const std::string_view& msg, TArgs... args) ->
				decltype(std::declval<State_t>().emplace<T>(std::move(args)...), void())
			{
				SetUpdateStatus(location, status, std::move(msg));
				DebugLog(__FUNCSIG__);
				m_Variant.emplace<T>(std::move(args)...);
			}

			template<typename T>
			auto Set(const mh::source_location& location, UpdateStatus status, const std::string_view& msg, T value) ->
				decltype(Emplace<T>(location, status, msg, std::move(value)), void())
			{
				Emplace<T>(location, status, msg, std::move(value));
			}

			void Clear(const mh::source_location& location, UpdateStatus status, const std::string_view& msg)
			{
				Emplace<std::monostate>(location, status, msg);
			}
			void ClearUpdateCheck(const mh::source_location& location, UpdateStatus status, const std::string_view& msg)
			{
				SetUpdateStatus(location, status, msg);
				DebugLog(MH_SOURCE_LOCATION_CURRENT()); // <- ??
				m_UpdateCheckVariant.emplace<0>();
			}

			template<typename T>
			void SetUpdateCheck(const mh::source_location& location, UpdateStatus status, const std::string_view& msg, T value)
			{
				SetUpdateStatus(location, status, msg);
				DebugLog(__FUNCSIG__);
				m_UpdateCheckVariant.emplace<T>(std::forward<T>(value));
			}

			void SetUpdateStatus(const mh::source_location& location, UpdateStatus status,
				const std::string_view& msg);
			mh::status_reader<UpdateStatus> GetUpdateStatus() const { return m_UpdateStatus; }

			template<typename TFutureResult> bool Update(const mh::source_location& location,
				UpdateStatus success, const std::string_view& successMsg,
				UpdateStatus failure, const std::string_view& failureMsg)
			{
				return UpdateVariant<TFutureResult>(m_Variant, location,
					success, successMsg, failure, failureMsg);
			}

		private:
			template<typename TFutureResult, typename TVariant>
			bool UpdateVariant(TVariant& variant, const mh::source_location& location,
				UpdateStatus success, const std::string_view& successMsg,
				UpdateStatus failure, const std::string_view& failureMsg);

			UpdateCheckState_t m_UpdateCheckVariant;
			State_t m_Variant;
			mh::status_source<UpdateStatus> m_UpdateStatus;

		} m_State;

		bool CanReplaceUpdateCheckState() const;


		bool m_IsUpdateQueued = true;
		bool m_IsInstalled = false;
	};

	UpdateManager::UpdateManager(const Settings& settings) :
		m_Settings(settings)
	{

		assert(m_IsUpdateQueued);
		m_State.SetUpdateStatus(MH_SOURCE_LOCATION_CURRENT(),
			UpdateStatus::CheckQueued, "Initializing update check...");
	}

	template<typename TFutureResult, typename TVariant>
	bool UpdateManager::StateManager::UpdateVariant(TVariant& variant, const mh::source_location& location,
		UpdateStatus success, const std::string_view& successMsg,
		UpdateStatus failure, const std::string_view& failureMsg)
	{
		if (auto future = std::get_if<std::future<std::optional<TFutureResult>>>(&variant))
		{
			try
			{
				if (mh::is_future_ready(*future))
				{
					auto value = future->get().value();
					SetUpdateStatus(MH_SOURCE_LOCATION_CURRENT(), success, std::string(successMsg));
					DebugLog(MH_SOURCE_LOCATION_CURRENT());
					variant.template emplace<TFutureResult>(std::move(value));
				}
			}
			catch (...)
			{
				const mh::exception_details details(std::current_exception());
				LogException(MH_SOURCE_LOCATION_CURRENT(), __FUNCSIG__);
				SetUpdateStatus(MH_SOURCE_LOCATION_CURRENT(), failure,
					mh::format("{}:\n\t- {}\n\t- {}", failureMsg, details.type_name(), details.m_Message));
				variant.template emplace<std::monostate>();
			}

			return true;
		}

		return false;
	}

	void UpdateManager::Update()
	{
		if (m_IsUpdateQueued && CanReplaceUpdateCheckState())
		{
			if (auto client = m_Settings.GetHTTPClient())
			{
				const auto releaseChannel = m_Settings.m_ReleaseChannel.value_or(ReleaseChannel::Public);

				if (releaseChannel != ReleaseChannel::None)
				{
					auto sharedClient = client->shared_from_this();

					m_State.SetUpdateCheck(
						MH_SOURCE_LOCATION_CURRENT(),
						UpdateStatus::Checking,
						"Checking for updates...",
						std::async([sharedClient, releaseChannel]() -> BuildInfo
							{
								auto newver = tf2_bot_detector::GithubAPI::CheckForNewVersion(*sharedClient);
								tf2_bot_detector::GithubAPI::NewVersionResult result = newver.get();

								BuildInfo ret;

								// new stable release
								if (result.m_Stable) {
									ret.m_GitHubURL = result.m_Stable->m_URL;
									ret.m_Version = result.m_Stable->m_Version;
								}

								// new pre-release
								if (releaseChannel >= ReleaseChannel::Preview && result.m_Preview) {
									ret.m_ReleaseChannel = ReleaseChannel::Preview;
									ret.m_GitHubURL = result.m_Preview->m_URL;
									ret.m_Version = result.m_Preview->m_Version;
								}

								return ret;
							}));
				}
				else {
					m_State.Emplace<std::monostate>(MH_SOURCE_LOCATION_CURRENT(), UpdateStatus::UpdateCheckDisabled,
						"Update checks disabled by user.");
				}
			}
			else
			{
				m_State.Emplace<std::monostate>(MH_SOURCE_LOCATION_CURRENT(), UpdateStatus::InternetAccessDisabled,
					"Update check skipped because internet connectivity was disabled by the user.");
			}

			m_IsUpdateQueued = false;
		}

		if (auto future = std::get_if<std::future<BuildInfo>>(&m_State.GetUpdateCheckVariant()))
		{
			try
			{
				auto client = m_Settings.GetHTTPClient();
				if (client)
				{
					if (mh::is_future_ready(*future))
					{
						AvailableUpdate update(*this, future->get());

						if (update.m_BuildInfo.m_Version <= VERSION)
						{
							m_State.SetUpdateCheck(MH_SOURCE_LOCATION_CURRENT(), UpdateStatus::UpToDate,
								mh::format("Up to date (v{} {:v})", VERSION, mh::enum_fmt(
									m_Settings.m_ReleaseChannel.value_or(ReleaseChannel::Public))),
								std::move(update));
						}
						else
						{
							m_State.SetUpdateCheck(MH_SOURCE_LOCATION_CURRENT(), UpdateStatus::UpdateAvailable,
								mh::format("Update available (v{} {:v})", update.m_BuildInfo.m_Version, mh::enum_fmt(
									m_Settings.m_ReleaseChannel.value_or(ReleaseChannel::Public))),
								std::move(update));
						}
					}
				}
				else
				{
					m_State.ClearUpdateCheck(MH_SOURCE_LOCATION_CURRENT(), UpdateStatus::CheckFailed,
						"Update check failed: HTTPClient unavailable");
				}
			}
			catch (const std::exception& e)
			{
				m_State.ClearUpdateCheck(MH_SOURCE_LOCATION_CURRENT(), UpdateStatus::CheckFailed,
					mh::format("Update check failed:\n\t- {}\n\t- {}", typeid(e).name(), e.what()));
			}
		}
	}

	auto UpdateManager::GetAvailableUpdate() const -> const AvailableUpdate*
	{
		return std::get_if<AvailableUpdate>(&m_State.GetUpdateCheckVariant());
	}

	/// <summary>
	/// Is it safe to replace the value of m_State? (make sure we won't block the thread)
	/// </summary>
	bool UpdateManager::CanReplaceUpdateCheckState() const
	{
		const auto* future = std::get_if<std::future<BuildInfo>>(&m_State.GetUpdateCheckVariant());
		if (!future)
			return true; // some other value in the variant

		if (!future->valid())
			return true; // future is empty

		if (mh::is_future_ready(*future))
			return true; // future is ready

		return false;
	}


	UpdateManager::AvailableUpdate::AvailableUpdate(UpdateManager& parent, BuildInfo&& buildInfo) :
		IAvailableUpdate(std::move(buildInfo)),
		m_Parent(parent)
	{
	}

	UpdateManager::BaseExceptionData::BaseExceptionData(const std::type_info& type, std::string message,
		const std::exception_ptr& exception) :
		m_Type(type), m_Message(std::move(message)), m_Exception(exception)
	{
	}

	void UpdateManager::StateManager::SetUpdateStatus(
		const mh::source_location& location, UpdateStatus status, const std::string_view& msg)
	{
		if (m_UpdateStatus.set(status, msg))
			DebugLog(location, "{}: {}", mh::enum_fmt(status), msg);
	}
}

std::unique_ptr<IUpdateManager> tf2_bot_detector::IUpdateManager::Create(const Settings& settings)
{
	return std::make_unique<UpdateManager>(settings);
}
