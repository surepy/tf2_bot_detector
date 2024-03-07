#include "UI/ImGui_TF2BotDetector.h"

#include <Util/ScopeGuards.h>

#include <regex>
#include <sstream>
#include <stdexcept>

#include "ClientReachedServerSpawnLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

std::shared_ptr<IConsoleLine> ClientReachedServerSpawnLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	if (args.m_Text == "Client reached server_spawn."sv)
		return std::make_shared<ClientReachedServerSpawnLine>(args.m_Timestamp); 

	return nullptr;
}

void ClientReachedServerSpawnLine::Print(const PrintArgs& args) const
{
	ImGui::TextFmt("Client reached server_spawn.");
}
