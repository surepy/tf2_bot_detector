#pragma once

#include "Config/Settings.h"
#include "ISetupFlowPage.h"
#include "ReleaseChannel.h"

#include <optional>

namespace tf2_bot_detector
{
	class NetworkSettingsPage final : public ISetupFlowPage
	{
	public:
		ValidateSettingsResult ValidateSettings(const Settings& settings) const override;
		OnDrawResult OnDraw(const DrawState& ds) override;
		void Init(const InitState& is) override;
		bool CanCommit() const override;
		void Commit(const CommitState& cs) override;
		SetupFlowPage GetPage() const override { return SetupFlowPage::NetworkSettings; }

	private:
		struct
		{
			std::optional<bool> m_AllowInternetUsage;
			std::optional<ReleaseChannel> m_ReleaseChannel;
			SteamAPIMode m_SteamAPIMode = SteamAPIMode::Disabled;
			std::string m_SteamAPIKey;

			inline std::string GetSteamAPIKey() const
			{
				return (m_SteamAPIMode == SteamAPIMode::Direct) ? m_SteamAPIKey : std::string();
			}

			bool m_EnableSteamHistoryIntegration = false;
			std::string m_SteamHistoryAPIKey;
		} m_Settings;

	};
}
