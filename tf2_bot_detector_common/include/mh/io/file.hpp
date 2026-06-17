#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace mh
{
	template<typename TChar = char, typename Traits = std::char_traits<TChar>, typename Alloc = std::allocator<TChar>>
	std::basic_string<TChar, Traits, Alloc> read_file(const std::filesystem::path& path)
	{
		std::basic_ifstream<TChar, Traits> file;
		file.exceptions(std::ios::badbit | std::ios::failbit);
		file.open(path);

		file.seekg(0, std::ios::end);
		const auto length = file.tellg();
		file.seekg(0, std::ios::beg);

		std::basic_string<TChar, Traits, Alloc> retVal(length, '\0');
		file.read(retVal.data(), length);

		return retVal;
	}

	template<typename TChar = char, typename Traits = std::char_traits<TChar>>
	void write_file(const std::filesystem::path& path, const std::basic_string_view<TChar, Traits>& contents)
	{
		std::basic_ofstream<TChar, Traits> file;
		file.exceptions(std::ios::badbit | std::ios::failbit);
		file.open(path);

		file.write(contents.data(), contents.size());
	}

	template<typename TChar>
	auto write_file(const std::filesystem::path& path, const TChar* string) ->
		decltype(std::basic_string_view(string), void())
	{
		return write_file(path, std::basic_string_view(string));
	}

#ifdef MH_COMPILE_LIBRARY
	extern template std::string read_file<char>(const std::filesystem::path&);
	extern template std::wstring read_file<wchar_t>(const std::filesystem::path&);

	extern template void write_file<char>(const std::filesystem::path&, const std::string_view&);
	extern template void write_file<wchar_t>(const std::filesystem::path&, const std::wstring_view&);

#ifndef MH_BROKEN_UNICODE
#if __cpp_unicode_characters >= 200704
	extern template std::u16string read_file<char16_t>(const std::filesystem::path&);
	extern template std::u32string read_file<char32_t>(const std::filesystem::path&);
	extern template void write_file<char16_t>(const std::filesystem::path&, const std::u16string_view&);
	extern template void write_file<char32_t>(const std::filesystem::path&, const std::u32string_view&);
#endif

#if __cpp_char8_t >= 201811
	extern template std::u8string read_file<char8_t>(const std::filesystem::path&);
	extern template void write_file<char8_t>(const std::filesystem::path&, const std::u8string_view&);
#endif
#endif
#endif
}
