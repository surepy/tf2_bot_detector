#include "UI/ImGui_TF2BotDetector.h"

#include <imgui_desktop/ScopeGuards.h>

#include <regex>
#include <sstream>
#include <stdexcept>

#include "CvarlistConvarLine.h"

using namespace tf2_bot_detector;
using namespace std::string_literals;
using namespace std::string_view_literals;

CvarlistConvarLine::CvarlistConvarLine(time_point_t timestamp, std::string name, float value,
	std::string flagsList, std::string helpText) :
	BaseClass(timestamp), m_Name(std::move(name)), m_Value(value),
	m_FlagsList(std::move(flagsList)), m_HelpText(std::move(helpText))
{
}

std::shared_ptr<IConsoleLine> CvarlistConvarLine::TryParse(const ConsoleLineTryParseArgs& args)
{
	static const std::regex s_Regex(R"regex((\S+)\s+:\s+([-\d.]+)\s+:\s+(.+)?\s+:[\t ]+(.+)?)regex", std::regex::optimize);
	if (svmatch result; std::regex_match(args.m_Text.begin(), args.m_Text.end(), result, s_Regex))
	{
		float value;
		from_chars_throw(result[2], value);
		return std::make_shared<CvarlistConvarLine>(args.m_Timestamp, result[1].str(), value, result[3].str(), result[4].str());
	}

	return nullptr;
}

void CvarlistConvarLine::Print(const PrintArgs& args) const
{
	// TODO
	//ImGui::Text("\"%s\" = \"%s\"", m_Name.c_str(), m_ConvarValue.c_str());
}
