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

#include <array>
#include <type_traits>


#define IM_VEC2_CLASS_EXTRA \
	bool operator==(const ImVec2& other) const { return x == other.x && y == other.y; } \
	bool operator!=(const ImVec2& other) const { return x != other.x || y != other.y; } \
	ImVec2 operator*(float s) const { return ImVec2(x * s, y * s); } \
	ImVec2 operator+(const ImVec2& o) const { return ImVec2(x + o.x, y + o.y); }

#define IM_VEC4_CLASS_EXTRA \
	template<size_t N, typename = std::enable_if_t<N == 4>> \
	ImVec4(float (&array)[N]) : x(array[0]), y(array[1]), z(array[2]), w(array[3]) {} \
	ImVec4(const std::array<float, 4>& array) : x(array[0]), y(array[1]), z(array[2]), w(array[3]) {} \
	ImVec4 operator+(const ImVec4& other) const { return ImVec4(x + other.x, y + other.y, z + other.z, w + other.w); } \
	ImVec4 operator-(const ImVec4& other) const { return ImVec4(x - other.x, y - other.y, z - other.z, w - other.w); } \
	ImVec4 operator*(float scalar) const { return ImVec4(x * scalar, y * scalar, z * scalar, w * scalar); } \
	constexpr std::array<float, 4> to_array() const { return { x, y, z, w }; }
