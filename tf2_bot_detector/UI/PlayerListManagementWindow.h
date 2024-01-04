#pragma once
#include "Config/PlayerListJSON.h"

namespace tf2_bot_detector
{
	class TF2BDApplication;

	class PlayerListManagementWindow
	{
	public:

		PlayerListManagementWindow();

		void Draw();
		void DrawFileEntries(const PlayerListJSON::PlayerListFile* file);

		void Open() { bOpen = true; }
		
	private:
		TF2BDApplication& m_Application;
		bool bOpen = false;
	};
};
