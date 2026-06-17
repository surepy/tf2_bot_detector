#pragma once

#if __has_include(<version>)
#include <version>
#endif

#include <array>
#if __has_include(<bit>)
#include <bit>
#endif
#if (__cpp_impl_three_way_comparison >= 201907)
#include <compare>
#endif
#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <type_traits>

#include <immintrin.h>

#include <mh/data/bit_float.hpp>

namespace mh
{
	namespace detail::uint128_hpp
	{
		constexpr bool is_constant_evaluated()
		{
#if __cpp_lib_is_constant_evaluated >= 201811
			return std::is_constant_evaluated();
#else
			return true;
#endif
		}

		template<typename TFunc> static constexpr void debug([[maybe_unused]] const TFunc& f)
		{
#if __cpp_lib_is_constant_evaluated >= 201811
			//if (!detail::is_constant_evaluated())
			//	f();
#endif
		}

#if (defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))

#define MH_UINT128_ENABLE_PLATFORM_UINT128 1
#if defined(__GNUC__) || defined(__clang__)
		using platform_uint128_t = __uint128_t;
#endif

#endif

		template<typename T>
		static constexpr int countl_zero(T x) noexcept
		{
#if __cpp_lib_bitops >= 201907
			return std::countl_zero<T>(x);
#else
			constexpr int DIGITS = sizeof(T) * std::numeric_limits<unsigned char>::digits;
			if (x == T(0))
				return DIGITS;

#if defined(__GNUC__) || defined(__clang__)
			if constexpr (std::is_same_v<T, uint64_t>)
				return __builtin_clzl(x);
			else
				return __builtin_clz(x);
#else
			int bits = 0;
			for (T i = (T(1) << (DIGITS - 1)); i != 0; i >>= 1)
			{
				if (i & x)
					break;

				bits++;
			}

			return bits;
#endif
#endif
		}
	}

	union uint128
	{
	public:
		template<unsigned i>
		constexpr uint64_t get_u64() const
		{
			static_assert(i == 0 || i == 1);
			return u64[i];
		}
		template<unsigned i>
		constexpr void set_u64(uint64_t val)
		{
			static_assert(i == 0 || i == 1);
			u64[i] = val;
		}

		explicit constexpr uint128(uint64_t low_ = 0, uint64_t high_ = 0)
		{
			set_u64<0>(low_);
			set_u64<1>(high_);
		}

		static constexpr uint128 from_mul(uint64_t a, uint64_t b)
		{
			uint128 retVal;
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
				retVal.u128 = a;
				retVal.u128 *= b;
				return retVal;
#endif
			}

			// https://stackoverflow.com/a/28904636/871842
			retVal.set_u64<0>(a * b);

			const auto aLow = a & 0xFFFFFFFF;
			const auto aHigh = a >> 32;
			const auto bLow = b & 0xFFFFFFFF;
			const auto bHigh = b >> 32;

			const auto resultLow = aLow * bLow;
			const auto resultHigh = aHigh * bHigh;
			const auto resultMidAB = aHigh * bLow;
			const auto resultMidBA = bHigh * aLow;

			const auto carryBit = ((resultMidAB & 0xFFFFFFFF) + (resultMidBA & 0xFFFFFFFF) + (resultLow >> 32)) >> 32;

			retVal.set_u64<1>(resultHigh + (resultMidAB >> 32) + (resultMidBA >> 32) + carryBit);

			return retVal;
		}

		constexpr uint128& operator++()
		{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
				u128++;
			}
			else
#endif
			{
				set_u64<0>(get_u64<0>() + 1);
				if (get_u64<0>() == 0)
					set_u64<1>(get_u64<1>() + 1);
			}

			return *this;
		}
		constexpr uint128 operator++(int)
		{
			uint128 temp = *this;
			++(*this);
			return temp;
		}

		constexpr uint128& operator+=(uint64_t rhs) { return *this = *this + rhs; }
		constexpr uint128 operator+(uint64_t rhs) const
		{
			uint128 retVal;
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
				retVal.u128 = u128 + rhs;
				return retVal;
#endif
			}

			const auto u64_low = get_u64<0>();
			const auto result = u64_low + rhs;
			retVal.set_u64<0>(result);
			retVal.set_u64<1>(get_u64<1>() + (result < u64_low ? 1 : 0));
			return retVal;
		}

		constexpr uint128& operator-=(uint64_t rhs)
		{
			const auto u64_low = get_u64<0>();
			const auto result = u64_low - rhs;

			set_u64<0>(result);
			set_u64<1>(get_u64<1>() - (result > u64_low ? 1 : 0));
			return *this;
		}
		constexpr uint128 operator-(const uint128& rhs) const { return uint128(*this) -= rhs; }
		constexpr uint128& operator-=(const uint128& rhs)
		{
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
				u128 -= rhs.u128;
				return *this;
#endif
			}

			const auto lhs_low = get_u64<0>();
			const auto rhs_low = rhs.get_u64<0>();
			const auto result_low = lhs_low - rhs_low;

			const auto lhs_high = get_u64<1>();
			const auto rhs_high = rhs.get_u64<1>();
			const auto result_high = (lhs_high - rhs_high) - (result_low > lhs_low ? 1 : 0);
			set_u64<0>(result_low);
			set_u64<1>(result_high);

			return *this;
		}

		constexpr uint8_t leading_zeros() const
		{
			if (auto z = detail::uint128_hpp::countl_zero(u64[1]); z != 64)
				return z;

			return 64 + detail::uint128_hpp::countl_zero(u64[0]);
		}

		template<typename T> constexpr uint128 operator<<(T bits) const
		{
			uint128 retVal;
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
				retVal.u128 = u128 << bits;
				return retVal;
#endif
			}

			static_assert(std::is_integral_v<T>);
			if (bits == 0)
			{
				retVal = *this;
			}
			else if (bits >= 128)
			{
				// retVal = 0
			}
			else if (bits < 0)
			{
				throw "uint128: operator<<: bits cannot be less than zero";
			}
			else if (bits <= 63)
			{
				retVal.set_u64<0>(get_u64<0>() << bits);
				retVal.set_u64<1>((get_u64<1>() << bits) | (get_u64<0>() >> (64 - bits)));
			}
			else if (bits <= 127)
			{
				retVal.set_u64<1>(get_u64<0>() << (bits - 64));
			}
			else
			{
				throw "Should never get here...";
			}

			return retVal;
		}
		template<typename T> constexpr uint128& operator<<=(T bits) { return *this = (*this << bits); }
		template<typename T> constexpr uint128 operator>>(T bits) const
		{
			uint128 retVal;
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
				retVal.u128 = u128 >> bits;
				return retVal;
#endif
			}

#if true
			static_assert(std::is_integral_v<T>);
			if (bits == 0)
			{
				retVal = *this;
			}
			else if (bits >= 128)
			{
				// retVal = 0
			}
			else if (bits < 0)
			{
				throw "uint128: operator>>: bits cannot be less than zero";
			}
			else if (bits <= 63)
			{
				retVal.set_u64<0>((get_u64<0>() >> bits) | (get_u64<1>() << (64 - bits)));
				retVal.set_u64<1>(get_u64<1>() >> bits);
			}
			else if (bits <= 127)
			{
				retVal.set_u64<0>(get_u64<1>() >> (bits - 64));
			}
			else
			{
				throw "Should never get here...";
			}
#else
			const uint8_t upper = bits / 2;
			const uint8_t lower = 1 - upper;
			const uint64_t low_new_low = ((get_u64<0>() >> (bits % 64)) * lower);
			retVal.set_u64<0>(((get_u64<0>() >> (bits % 64)) * lower) |
#endif

				return retVal;
		}

	public:
		constexpr uint128 operator/(uint64_t divisor) const;

		std::array<uint64_t, 2> u64{};
		std::array<uint32_t, 4> u32;
		std::array<uint16_t, 8> u16;
		std::array<uint8_t, 16> u8;

#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
		detail::uint128_hpp::platform_uint128_t u128;

		constexpr detail::uint128_hpp::platform_uint128_t get_u128() const
		{
			if (!detail::uint128_hpp::is_constant_evaluated())
				return u128;

#if __cpp_lib_bit_cast >= 201806
			return detail::bit_float_hpp::bit_cast<detail::uint128_hpp::platform_uint128_t>(u64);
#else
			return (detail::uint128_hpp::platform_uint128_t(get_u64<1>()) << 64) | get_u64<0>();
#endif
		}
		constexpr void set_u128(detail::uint128_hpp::platform_uint128_t value)
		{
			if (!detail::uint128_hpp::is_constant_evaluated())
			{
				u128 = value;
				return;
			}

#if __cpp_lib_bit_cast >= 201806
			u64 = detail::bit_float_hpp::bit_cast<std::array<uint64_t, 2>>(value);
#else
			set_u64<0>(value);
			set_u64<1>(value >> 64);
#endif
		}
#endif
	};

	inline constexpr bool operator==(const mh::uint128& lhs, const mh::uint128& rhs)
	{
		return lhs.get_u64<0>() == rhs.get_u64<0>() && lhs.get_u64<1>() == rhs.get_u64<1>();
	}
	inline constexpr bool operator!=(const mh::uint128& lhs, const mh::uint128& rhs)
	{
		return !(lhs == rhs);
	}

#if (__cpp_impl_three_way_comparison >= 201907)
	inline constexpr std::strong_ordering operator<=>(
		const mh::uint128& lhs, const mh::uint128& rhs)
	{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
		return lhs.get_u128() <=> rhs.get_u128();
#else
		if (auto result = lhs.get_u64<1>() <=> rhs.get_u64<1>(); std::is_neq(result))
			return result;

		return lhs.get_u64<0>() <=> rhs.get_u64<0>();
#endif
	}

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr std::strong_ordering operator<=>(const mh::uint128& lhs, T rhs)
	{
		if (!mh::detail::uint128_hpp::is_constant_evaluated())
		{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
			return lhs.u128 <=> mh::detail::uint128_hpp::platform_uint128_t(rhs);
#endif
		}

		if (auto result = lhs.get_u64<1>() <=> 0; std::is_neq(result))
			return result;

		return lhs.get_u64<0>() <=> rhs;
	}

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr std::strong_ordering operator<=>(T lhs, const mh::uint128& rhs)
	{
		if (!mh::detail::uint128_hpp::is_constant_evaluated())
		{
#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
			return mh::detail::uint128_hpp::platform_uint128_t(lhs) <=> rhs.u128;
#endif
		}

		if (auto result = 0 <=> rhs.get_u64<1>(); std::is_neq(result))
			return result;

		return lhs <=> rhs.get_u64<0>();
	}
#else
	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator<(const mh::uint128& lhs, T rhs)
	{
		return !lhs.get_u64<1>() && lhs.get_u64<0>() < rhs;
	}
	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator<(T lhs, const mh::uint128& rhs)
	{
		return rhs.get_u64<1>() || lhs < rhs.get_u64<0>();
	}
	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator>=(const mh::uint128& lhs, T rhs)
	{
		return !(lhs < rhs);
	}
#endif

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator==(const mh::uint128& lhs, T rhs)
	{
		if constexpr (sizeof(T) <= sizeof(uint64_t))
			return !lhs.get_u64<1>() && lhs.get_u64<0>() == rhs;
		else
			return lhs == mh::uint128(rhs);
	}
	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator==(T lhs, const mh::uint128& rhs)
	{
		return rhs == lhs;
	}

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator!=(const mh::uint128& lhs, T rhs)
	{
		return !(lhs == rhs);
	}
	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr bool operator!=(T lhs, const mh::uint128& rhs)
	{
		return !(lhs == rhs);
	}

	template<typename CharT, typename Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const mh::uint128& rhs)
	{
		return os << '['
			<< std::hex << rhs.template get_u64<1>() << '|'
			<< std::hex << rhs.template get_u64<0>() << ']';
	}

	inline constexpr mh::uint128 mh::uint128::operator/(uint64_t divisor) const
	{
		uint128 quotient;

#ifdef MH_UINT128_ENABLE_PLATFORM_UINT128
		quotient.set_u128(get_u128() / divisor);
#else
		// It's slow, but it works
		if (get_u64<1>() == 0)
		{
			quotient.set_u64<0>(get_u64<0>() / divisor);
		}
		else if (*this >= divisor)
		{
			uint64_t remainder64 = get_u64<1>() % divisor;
			quotient.set_u64<1>(get_u64<1>() / divisor);
			uint64_t quotientLow = 0;

			uint64_t buffer = get_u64<0>();

			const uint8_t skip = remainder64 ? 0 : detail::uint128_hpp::countl_zero(buffer);
			buffer <<= skip;
			uint8_t count = 64 - skip;

			const auto print_bin = [](uint64_t val) -> const char*
			{
				for (int i = 0; i < 64; i++)
					std::cerr << ((val & (uint64_t(1) << (63 - i))) ? '1' : '_');

				return "";
			};

			const auto print_vars = [&]
			{
				detail::uint128_hpp::debug([&]
					{
						std::cerr
							<< "\nquotient:     " << print_bin(quotientLow)
							<< "\nbuffer:       " << print_bin(buffer)
							<< "\nremainder64:  " << print_bin(remainder64)
							<< "\ndivisor:      " << print_bin(divisor)
							<< "\n";
					});
			};

			detail::uint128_hpp::debug([] { std::cerr << "Initial value:\n"; });
			print_vars();

			if (divisor & (uint64_t(1) << 63))
			{
				while (count--)
				{
					const uint64_t high_bit = remainder64 & (uint64_t(1) << 63);
					remainder64 <<= 1;
					remainder64 |= (buffer >> 63);
					buffer <<= 1;
					//quotientLow <<= 1;

					if (high_bit || remainder64 >= divisor)
					{
						remainder64 -= divisor;
						quotientLow |= uint64_t(1) << count;
					}
				}
			}
			else
			{
				while (count--)
				{
					remainder64 <<= 1;
					remainder64 |= (buffer >> 63);
					buffer <<= 1;
					//quotientLow <<= 1;

					if (remainder64 >= divisor)
					{
						detail::uint128_hpp::debug([&] { std::cerr << remainder64 << " >= " << divisor << '\n'; });
						//const auto test = remainder64 >= divisor;
						remainder64 -= divisor;
						quotientLow |= uint64_t(1) << count;
					}

					print_vars();
				}
				print_vars();
			}

			quotient.set_u64<0>(quotientLow);
		}
#endif

		return quotient;
	}
}
