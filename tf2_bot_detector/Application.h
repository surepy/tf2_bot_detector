#pragma once

#include <imgui_desktop/Application.h>

#include <memory>

namespace tf2_bot_detector
{
	class MainWindow;

	namespace DB
	{
		class ITempDB;
	}

	class TF2BDApplication
	{
	public:
		TF2BDApplication();
		~TF2BDApplication();

		static TF2BDApplication& GetApplication();

		DB::ITempDB& GetTempDB();

	private:
		MainWindow* m_MainWindow = nullptr;

		std::unique_ptr<DB::ITempDB> m_TempDB;
	};
}
