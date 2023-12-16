#pragma once

namespace tf2_bot_detector
{
	class MainWindow;
	class Settings;

	class SettingsWindow 
	{
	public:
		SettingsWindow(Settings& settings, MainWindow& mainWindow);

		void OnDraw();

	private:
		void OnDrawASOSettings();
		void OnDrawCompatibilitySettings();
		void OnDrawLoggingSettings();
		void OnDrawModerationSettings();
		void OnDrawModSettings();
		void OnDrawPerformanceSettings();
		void OnDrawServiceIntegrationSettings();
		void OnDrawUISettings();
		void OnDrawMiscSettings();

		Settings& m_Settings;
		MainWindow& m_MainWindow;

		bool m_ModsChanged = false;
	};
}
