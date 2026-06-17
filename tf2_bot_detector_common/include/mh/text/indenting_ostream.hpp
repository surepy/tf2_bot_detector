#pragma once

#include <cassert>
#include <ostream>

namespace mh
{
	template<typename CharT, typename Traits>
	class indenting_ostream : std::basic_streambuf<CharT, Traits>, public std::basic_ostream<CharT, Traits>
	{
	public:
		using ostream_type = std::basic_ostream<CharT, Traits>;
		using streambuf_type = std::basic_streambuf<CharT, Traits>;
		using char_type = CharT;
		using int_type = typename streambuf_type::int_type;

		indenting_ostream(ostream_type& underlyingStream, CharT indentChar = '\t', size_t indentCount = 1, CharT newlineChar = '\n') :
			ostream_type(this),
			m_UnderlyingStream(underlyingStream),
			m_NewlineChar(newlineChar),
			m_IndentChar(indentChar),
			m_IndentCount(indentCount)
		{
		}

		void set_indent_count(size_t count) { m_IndentCount = count; }
		size_t get_indent_count() const { return m_IndentCount; }

		void set_indent_char(CharT indentChar) { m_IndentChar = indentChar; }
		CharT get_indent_char() const { return m_IndentChar; }

	protected:
		std::streamsize xsputn(const CharT* s, std::streamsize countSigned) override
		{
			if (countSigned < 0)
				throw std::invalid_argument("count cannot be less than zero");

			size_t count = static_cast<size_t>(countSigned);

			// Just for debugging
			[[maybe_unused]] const auto originalS = s;
			const auto originalCount = count;

			while (count > 0)
			{
				bool wroteAny = false;

				// Find the next newline char
				for (size_t i = 0; i < count; i++)
				{
					if (s[i] == m_NewlineChar)
					{
						const auto offset = i + 1;
						m_UnderlyingStream.write(s, offset);
						write_indent_chars();

						assert(offset <= count);
						count -= offset;
						s += offset;
						wroteAny = true;
						break;
					}
				}

				if (!wroteAny)
				{
					// Write the remainder
					m_UnderlyingStream.write(s, count);
					break;
				}
			}

			return originalCount;
		}

		int_type overflow(int_type ch = Traits::eof()) override
		{
			const auto asChar = Traits::to_char_type(ch);
			m_UnderlyingStream.put(asChar);
			if (Traits::eq(m_NewlineChar, asChar))
				write_indent_chars();

			return ch;
		}

	private:
		void write_indent_chars()
		{
			for (size_t i = 0; i < m_IndentCount; i++)
				m_UnderlyingStream.put(m_IndentChar);
		}

		ostream_type& m_UnderlyingStream;
		CharT m_NewlineChar;
		CharT m_IndentChar;
		size_t m_IndentCount;
	};

	template<typename TObject, typename CharT = char>
	struct indented
	{
		constexpr indented(const TObject& obj, CharT indentChar = '\t', size_t indentCount = 1, CharT newlineChar = '\n') :
			m_Object(obj),
			m_IndentChar(indentChar),
			m_IndentCharCount(indentCount),
			m_NewlineChar(newlineChar)
		{
		}

		CharT m_IndentChar;
		CharT m_NewlineChar;
		size_t m_IndentCharCount;
		const TObject& m_Object;
	};

	template<typename CharT, typename Traits, typename TObject>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const indented<TObject, CharT>& obj)
	{
		mh::indenting_ostream<CharT, Traits> indenting_os(os, obj.m_IndentChar, obj.m_IndentCharCount, obj.m_NewlineChar);
		return indenting_os << obj.m_Object;
	}
}
