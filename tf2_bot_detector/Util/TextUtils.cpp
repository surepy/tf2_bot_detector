#include "TextUtils.h"
#include "Log.h"

#include <mh/text/codecvt.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/text/string_insertion.hpp>

#include <codecvt>
#include <fstream>

using namespace std::string_literals;

// TODO: move to platform? might make more sense over there

std::u16string tf2_bot_detector::ToU16(const std::u8string_view& input)
{
	const char* dataTest = reinterpret_cast<const char*>(input.data());
	return mh::change_encoding<char16_t>(input);
}

std::u16string tf2_bot_detector::ToU16(const char* input, const char* input_end)
{
	if (input_end)
		return ToU16(std::string_view(input, input_end - input));
	else
		return ToU16(std::string_view(input));
}

std::u16string tf2_bot_detector::ToU16(const std::string_view& input)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.from_bytes(input.data(), input.data() + input.size());
}

std::u16string tf2_bot_detector::ToU16(const std::wstring_view& input)
{
	return mh::change_encoding<char16_t>(input);
	//return std::u16string(std::u16string_view(reinterpret_cast<const char16_t*>(input.data()), input.size()));
}

std::u8string tf2_bot_detector::ToU8(const std::string_view& input)
{
	return mh::change_encoding<char8_t>(input);
	//return std::u8string(std::u8string_view(reinterpret_cast<const char8_t*>(input.data()), input.size()));
}

std::u8string tf2_bot_detector::ToU8(const std::u16string_view& input)
{
	return mh::change_encoding<char8_t>(input);
}

std::u8string tf2_bot_detector::ToU8(const std::wstring_view& input)
{
	return mh::change_encoding<char8_t>(input);
	//return ToU8(std::u16string_view(reinterpret_cast<const char16_t*>(input.data()), input.size()));
}

std::string tf2_bot_detector::ToMB(const std::u8string_view& input)
{
	return mh::change_encoding<char>(input);
	//return std::string(std::string_view(reinterpret_cast<const char*>(input.data()), input.size()));
}

std::string tf2_bot_detector::ToMB(const std::u16string_view& input)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(input.data(), input.data() + input.size());
}

std::string tf2_bot_detector::ToMB(const std::wstring_view& input)
{
	return mh::change_encoding<char>(input);
	//return ToMB(ToU16(input));
}

std::wstring tf2_bot_detector::ToWC(const std::string_view& input)
{
	return mh::change_encoding<wchar_t>(input);
}

std::u16string tf2_bot_detector::ReadWideFile(const std::filesystem::path& filename)
{
	DebugLog("ReadWideFile("s << filename << ')');

	std::u16string wideFileData;
	{
		std::ifstream file;
		file.exceptions(std::ios::badbit | std::ios::failbit);
		file.open(filename, std::ios::binary);

		// read BOM.
		char bom[3];
		file.read(bom, 3);

		// UTF-8 bom, we should probably NOT use this function.
		if (bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF') {
			LogException("UTF8 BOM detected in file {}", filename.string());
			throw std::runtime_error("UTF8 BOM detected");
		}

		file.seekg(0, std::ios::end);

		// Length, minus BOM
		const auto length = static_cast<size_t>(file.tellg()) - sizeof(char16_t);

		// Skip BOM
		file.seekg(2, std::ios::beg);

		wideFileData.resize(length);

		file.read(reinterpret_cast<char*>(wideFileData.data()), length);
	}

	return wideFileData;
}

void tf2_bot_detector::WriteWideFile(const std::filesystem::path& filename, const std::u16string_view& text)
{
	std::ofstream file;
	file.exceptions(std::ios::badbit | std::ios::failbit);
	file.open(filename, std::ios::binary);
	file << '\xFF' << '\xFE'; // BOM - UTF16LE

	file.write(reinterpret_cast<const char*>(text.data()), text.size() * sizeof(text[0]));
}

std::string tf2_bot_detector::CollapseNewlines(const std::string_view& input)
{
	std::string retVal;

	// collapse groups of newlines in the message into red "(\n x <count>)" text
	bool firstLine = true;
	for (size_t i = 0; i < input.size(); )
	{
		size_t nonNewlineEnd = std::min(input.find('\n', i), input.size());
		retVal.append(input.substr(i, nonNewlineEnd - i));

		size_t newlineEnd = std::min(input.find_first_not_of('\n', nonNewlineEnd), input.size());

		if (newlineEnd > nonNewlineEnd)
		{
			const auto newlineCount = (newlineEnd - nonNewlineEnd);

			const auto smallGroupMsgLength = newlineCount * (std::size("\\n") - 1);

			const mh::fmtstr<64> newlineGroupStr("(\\n x %zu)", newlineCount);
			if (smallGroupMsgLength >= newlineGroupStr.size())
			{
				retVal.append(newlineGroupStr);
			}
			else
			{
				for (size_t n = 0; n < newlineCount; n++)
					retVal += "\\n";
			}
		}

		i = newlineEnd;
		firstLine = false;
	}

	return retVal;
}
