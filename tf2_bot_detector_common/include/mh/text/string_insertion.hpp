#pragma once

#include <ostream>
#include <streambuf>
#include <string>

namespace mh
{
	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	class basic_strwrapperstream final : std::basic_streambuf<CharT, Traits>, public std::basic_ostream<CharT, Traits>
	{
	public:
		using char_type = CharT;
		using ostream_type = std::basic_ostream<CharT, Traits>;
		using string_type = std::basic_string<CharT, Traits, Alloc>;
		using streambuf_type = std::basic_streambuf<CharT, Traits>;
		using int_type = typename streambuf_type::int_type;

		basic_strwrapperstream(std::basic_string<CharT, Traits, Alloc>& string) :
			ostream_type(this),
			m_String(string)
		{
			ostream_type::exceptions(std::ios::badbit | std::ios::failbit);
		}

	protected:
		std::streamsize xsputn(const CharT* s, std::streamsize count) override
		{
			m_String.append(s, s + count);
			return count;
		}

		int_type overflow(int_type ch = Traits::eof()) override
		{
			if (ch != Traits::eof())
				m_String.push_back(static_cast<CharT>(ch));

			return ch;
		}

	private:
		std::basic_string<CharT, Traits, Alloc>& m_String;
	};

#ifdef MH_COMPILE_LIBRARY
	extern template class basic_strwrapperstream<char>;
	extern template class basic_strwrapperstream<wchar_t>;
#endif

	using strwrapperstream = basic_strwrapperstream<>;

	namespace detail::string_insertion_hpp
	{
		template<typename T>
		struct make_dependent
		{
			using type = T;
		};

		// Force dependent typename for T, so we can use stream insertion operators declared after ourselves
		template<typename T, typename CharT, typename Traits, typename Alloc>
		inline void insertion_op_impl(std::basic_string<CharT, Traits, Alloc>& str, const typename make_dependent<T>::type& value)
		{
			mh::basic_strwrapperstream<CharT, Traits, Alloc> stream(str);

			if constexpr (std::is_same_v<bool, std::decay_t<T>>)
				stream << std::boolalpha;

			stream << value;
		}
	}
}

namespace std
{
	template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	inline auto operator<<(std::basic_string<CharT, Traits, Alloc>& str, const T& value)
		-> decltype(std::declval<std::basic_ostream<CharT, Traits>>() << value, str)
	{
		mh::detail::string_insertion_hpp::insertion_op_impl<T, CharT, Traits, Alloc>(str, value);
		return str;
	}

	template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
	inline auto operator<<(std::basic_string<CharT, Traits, Alloc>&& str, const T& value)
		-> decltype(std::declval<std::basic_ostream<CharT, Traits>>() << value, std::move(str))
	{
		mh::detail::string_insertion_hpp::insertion_op_impl<T, CharT, Traits, Alloc>(str, value);
		return std::move(str);
	}
}
