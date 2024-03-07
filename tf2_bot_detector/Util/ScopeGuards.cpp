/*
Pasted from imgui_desktop

MIT License

Copyright (c) 2020 Matt Haynie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ScopeGuards.h"

#include <imgui.h>

using namespace ImGuiDesktop::ScopeGuards;

ID::ID(int int_id)
{
	ImGui::PushID(int_id);
}

ID::ID(const void* ptr_id)
{
	ImGui::PushID(ptr_id);
}

ID::ID(const char* str_id_begin, const char* str_id_end)
{
	ImGui::PushID(str_id_begin, str_id_end);
}

ID::ID(const std::string_view& sv) : ID(sv.data(), sv.data() + sv.size())
{
}

ID::~ID()
{
	ImGui::PopID();
}

StyleColor::StyleColor(ImGuiCol_ color, const ImVec4& value, bool enabled) :
	m_Enabled(enabled)
{
	if (m_Enabled)
		ImGui::PushStyleColor(color, value);
}

StyleColor::~StyleColor()
{
	if (m_Enabled)
		ImGui::PopStyleColor();
}

StyleColor::StyleColor(StyleColor&& other) :
	m_Enabled(other.m_Enabled)
{
	other.m_Enabled = false;
}
StyleColor& StyleColor::operator=(StyleColor&& other)
{
	if (m_Enabled)
		ImGui::PopStyleColor();

	m_Enabled = other.m_Enabled;
	other.m_Enabled = false;
	return *this;
}

TextColor::TextColor(const ImVec4& color, bool enabled) :
	StyleColor(ImGuiCol_Text, color, enabled)
{
}

GlobalAlpha::GlobalAlpha(float newAlpha) :
	m_OldAlpha(ImGui::GetStyle().Alpha)
{
	auto& style = ImGui::GetStyle();
	m_OldAlpha = style.Alpha;
	style.Alpha = newAlpha;
}

GlobalAlpha::~GlobalAlpha()
{
	ImGui::GetStyle().Alpha = m_OldAlpha;
}

Indent::Indent(unsigned count) :
	m_Count(count)
{
	for (unsigned i = 0; i < count; i++)
		ImGui::Indent();
}
Indent::~Indent()
{
	for (unsigned i = 0; i < m_Count; i++)
		ImGui::Unindent();
}

Context::Context(ImGuiContext* newContext) :
	m_OldContext(ImGui::GetCurrentContext()),
	m_NewContext(newContext)
{
	ImGui::SetCurrentContext(m_NewContext);
}
Context::~Context()
{
	assert(ImGui::GetCurrentContext() == m_NewContext);
	ImGui::SetCurrentContext(m_OldContext);
}
