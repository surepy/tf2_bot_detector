#include "NetworkSettingsPage.h"
#include "Config/Settings.h"
#include "UI/ImGui_TF2BotDetector.h"
#include "ReleaseChannel.h"
#include "Version.h"
#include "UpdateManager.h"

#include <mh/error/ensure.hpp>

using namespace tf2_bot_detector;

template<typename T>
static bool InternalValidateSettings(const T& settings)
{
	if (!settings.m_AllowInternetUsage.has_value())
		return false;
	if (!settings.m_ReleaseChannel.has_value())
		return false;
	if (settings.m_SteamAPIMode == SteamAPIMode::Direct && settings.GetSteamAPIKey().empty())
		return false;

#if 0
	if (settings.m_ProgramUpdateCheckMode == ProgramUpdateCheckMode::Releases && VERSION.m_Preview != 0)
		return false;
#endif

	return true;
}

auto NetworkSettingsPage::ValidateSettings(const Settings& settings) const -> ValidateSettingsResult
{
	return InternalValidateSettings(settings) ? ValidateSettingsResult::Success : ValidateSettingsResult::TriggerOpen;
}

auto NetworkSettingsPage::OnDraw(const DrawState& ds) -> OnDrawResult
{
	ImGui::TextFmt("This tool can optionally connect to the internet to automatically update.");

	ImGui::Indent();
	{
		if (bool allow = m_Settings.m_AllowInternetUsage.value_or(false); ImGui::Checkbox("Allow Internet Connectivity", &allow))
			m_Settings.m_AllowInternetUsage = allow;

		if (m_Settings.m_AllowInternetUsage.value_or(false))
			ImGui::TextFmt({ 1, 1, 0, 1 }, "If you use antivirus software, connecting to the internet may trigger warnings.");
	}
	ImGui::Unindent();
	ImGui::NewLine();

	const bool enabled = m_Settings.m_AllowInternetUsage.value_or(false);
	ImGui::EnabledSwitch(enabled, [&](bool enabled)
		{
			ImGui::BeginGroup();
			ImGui::TextFmt("This tool can also check for updated functionality and bugfixes on startup.");
			ImGui::Indent();
			{
				std::optional<ReleaseChannel> mode = enabled ? m_Settings.m_ReleaseChannel : ReleaseChannel::None;
				if (Combo("##SetupFlow_UpdateCheckingMode", mode))
					m_Settings.m_ReleaseChannel = mode;

				if (m_Settings.m_ReleaseChannel == ReleaseChannel::None)
					ImGui::TextFmt("You can always check for updates manually via the Help menu.");
			}
			ImGui::Unindent();
			ImGui::EndGroup();
		}, "Requires \"Allow Internet Connectivity\"");


	ImGui::NewLine();
	ImGui::Text("Steam API Integration:\nIt is *VERY strongly* recommended you use tf2 bot detector with the Steam API integration enabled as they provide much more information and utility.");

	// copypasted from SettingsWindow.cpp#292, move somewhere else later.
	const auto GetSteamAPIModeString = [](SteamAPIMode mode)
	{
		switch (mode)
		{
		case SteamAPIMode::Disabled:  return "Disabled";
		case SteamAPIMode::Direct:    return "Enabled";
		}

		LogError("Unknown value {}", mh::enum_fmt(mode));
		return "Disabled";
	};

	ImGui::Indent();

	if (ImGui::BeginCombo("Steam API Mode", GetSteamAPIModeString(m_Settings.m_SteamAPIMode)))
	{
		const auto ModeSelectable = [&](SteamAPIMode mode)
		{
			if (ImGui::Selectable(GetSteamAPIModeString(mode), m_Settings.m_SteamAPIMode == mode))
			{
				m_Settings.m_SteamAPIMode = mode;
			}
		};

		ModeSelectable(SteamAPIMode::Disabled);
		ImGui::SetHoverTooltip("Disables the Steam API integration completely.");

		ModeSelectable(SteamAPIMode::Direct);
		ImGui::SetHoverTooltip("Communicates directly with the Steam API servers.");

		ImGui::EndCombo();
	}

	ImGui::NewLine();

	if (m_Settings.m_SteamAPIMode == SteamAPIMode::Direct) {
		InputTextSteamAPIKey("Steam API Key", m_Settings.m_SteamAPIKey, true);
	}

	ImGui::Unindent();

	return OnDrawResult::ContinueDrawing;
}

void NetworkSettingsPage::Init(const InitState& is)
{
	m_Settings.m_AllowInternetUsage = is.m_Settings.m_AllowInternetUsage.value_or(true);
	m_Settings.m_ReleaseChannel = is.m_Settings.m_ReleaseChannel;
	if (!m_Settings.m_ReleaseChannel.has_value())
		m_Settings.m_ReleaseChannel = ReleaseChannel::Public;
}

bool NetworkSettingsPage::CanCommit() const
{
	return InternalValidateSettings(m_Settings);
}

void NetworkSettingsPage::Commit(const CommitState& cs)
{
	const bool oldClient = !!cs.m_Settings.GetHTTPClient();
	cs.m_Settings.m_AllowInternetUsage = m_Settings.m_AllowInternetUsage;
	cs.m_Settings.m_ReleaseChannel = m_Settings.m_ReleaseChannel;

	if (!oldClient && cs.m_Settings.GetHTTPClient())
	{
		DebugLog(MH_SOURCE_LOCATION_CURRENT(), "Re-checking for updates, user has chosen to allow internet connectivity");

		// Trigger an update check, user has enabled
		if (mh_ensure(cs.m_UpdateManager))
			cs.m_UpdateManager->QueueUpdateCheck();
	}

	cs.m_Settings.m_SteamAPIMode = m_Settings.m_SteamAPIMode;
	cs.m_Settings.SetSteamAPIKey(m_Settings.m_SteamAPIKey);
}
