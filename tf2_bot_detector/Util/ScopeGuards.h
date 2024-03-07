#pragma once

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

#include <mh/types/disable_copy_move.hpp>

#include <string_view>
#include <imgui.h>

namespace ImGuiDesktop
{
	inline namespace ScopeGuards
	{
		struct ID final : mh::disable_copy_move
		{
		public:
			explicit ID(int int_id);
			explicit ID(const void* ptr_id);
			explicit ID(const char* str_id_begin, const char* str_id_end = nullptr);
			explicit ID(const std::string_view& sv);
			~ID();
		};

		struct StyleColor : mh::disable_copy
		{
			StyleColor() : m_Enabled(false) {}
			StyleColor(ImGuiCol_ color, const ImVec4& value, bool enabled = true);
			~StyleColor();

			StyleColor(StyleColor&& other);
			StyleColor& operator=(StyleColor&& other);

		private:
			bool m_Enabled = true;
		};

		struct TextColor final : StyleColor
		{
			TextColor(const ImVec4& color, bool enabled = true);
		};

		struct GlobalAlpha final : mh::disable_copy_move
		{
			explicit GlobalAlpha(float newAlpha);
			~GlobalAlpha();

		private:
			float m_OldAlpha;
		};

		struct Indent final : mh::disable_copy_move
		{
			explicit Indent(unsigned count = 1);
			~Indent();

		private:
			unsigned m_Count;
		};

		struct Context final : mh::disable_copy_move
		{
			explicit Context(ImGuiContext* newContext);
			~Context();

		private:
			ImGuiContext* m_OldContext;
			ImGuiContext* m_NewContext;
		};
	}
}
