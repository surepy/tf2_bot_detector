#pragma once

#if __has_include(<version>)
#include <version>
#endif

#if __cpp_lib_bit_cast >= 201806
#include <bit>
#endif

#include <climits>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <type_traits>

namespace mh
{
	namespace detail::bit_float_hpp
	{
		template<unsigned bits>
		static constexpr auto uint_for_bits()
		{
			static_assert(bits <= 64);
			if constexpr (bits > 32)
				return uint64_t{};
			else if constexpr (bits > 16)
				return uint32_t{};
			else if constexpr (bits > 8)
				return uint16_t{};
			else
				return uint8_t{};
		}
		template<unsigned bits> using uint_for_bits_t = decltype(uint_for_bits<bits>());
		template<unsigned bits> using int_for_bits_t = std::make_signed_t<uint_for_bits_t<bits>>;

		template<typename T>
		constexpr T bits_to_mask(T bits) { return (T(1) << bits) - 1; }

		constexpr unsigned HLF_MNT_BITS = 10;
		constexpr unsigned HLF_EXP_BITS = 5;
		constexpr unsigned DBL_MNT_BITS = std::numeric_limits<double>::digits - 1;
		constexpr unsigned DBL_EXP_BITS = 11;
		constexpr unsigned FLT_MNT_BITS = std::numeric_limits<float>::digits - 1;
		constexpr unsigned FLT_EXP_BITS = 8;

		constexpr bool is_constant_evaluated() noexcept
		{
#if __cpp_lib_is_constant_evaluated >= 201811
			return std::is_constant_evaluated();
#else
			return true;
#endif
		}

		template<typename To, typename From>
		constexpr To bit_cast(const From& from)
		{
#if __cpp_lib_bit_cast >= 201806
			return std::bit_cast<To>(from);
#else
			static_assert(sizeof(To) == sizeof(From));
			return *reinterpret_cast<const To*>(&from);
#endif
		}
	}

	template<unsigned bits>
	struct mantissa_t
	{
		static_assert(bits > 0, "Mantissa bits cannot be 0");
		static_assert(bits <= detail::bit_float_hpp::DBL_MNT_BITS, "Mantissa bits cannot be greater than those of a double");

		using value_t = detail::bit_float_hpp::uint_for_bits_t<bits>;
		static constexpr value_t BITS = bits;
		static constexpr value_t MASK = detail::bit_float_hpp::bits_to_mask<value_t>(bits);
		static constexpr mantissa_t<bits> max() { return mantissa_t<bits>(MASK); }
		static constexpr mantissa_t<bits> min() { return mantissa_t<bits>(0); }

		constexpr mantissa_t() = default;
		explicit constexpr mantissa_t(value_t v) : value(v & MASK) {}

		template<unsigned newBits>
		constexpr mantissa_t<newBits> convert() const
		{
			if constexpr (newBits == bits)
				return *this;
			else if constexpr (newBits > bits)
				return mantissa_t<newBits>(typename mantissa_t<newBits>::value_t(value) << (newBits - bits));
			else // newBits < MantissaBits
				return mantissa_t<newBits>(value >> (bits - newBits));
		}

		template<unsigned otherBits>
		constexpr bool operator==(mantissa_t<otherBits> other) const
		{
			if constexpr (bits == otherBits)
				return value == other.value;
			else if constexpr (bits > otherBits)
				return value == other.template convert<bits>().value;
			else if constexpr (bits < otherBits)
				return convert<otherBits>().value == other.value;
		}
		template<unsigned otherBits>
		constexpr bool operator!=(mantissa_t<otherBits> other) const { return !operator==(other); }

		value_t value{};
	};

	template<unsigned bits>
	struct exponent_t
	{
		static_assert(bits > 0, "Exponent bits cannot be 0");
		static_assert(bits <= detail::bit_float_hpp::DBL_EXP_BITS, "Exponent bits cannot be greater than those of a double");

		using value_t = detail::bit_float_hpp::uint_for_bits_t<bits>;
		static constexpr value_t BITS = bits;
		static constexpr value_t MASK = detail::bit_float_hpp::bits_to_mask<value_t>(bits);
		static constexpr value_t OFFSET = ((1 << bits) / 2) - 1;
		static constexpr exponent_t<bits> inf_or_nan() { return exponent_t<bits>(MASK); }
		static constexpr exponent_t<bits> max() { return exponent_t<bits>(MASK - 1); }
		static constexpr exponent_t<bits> zero() { return exponent_t<bits>(((1 << bits) / 2) - 1); }
		static constexpr exponent_t<bits> min() { return exponent_t<bits>(1); }

		constexpr exponent_t() = default;
		explicit constexpr exponent_t(value_t v) : value(v & MASK) {}

		constexpr auto actual_value() const
		{
			using t = detail::bit_float_hpp::int_for_bits_t<bits + 1>;
			return t(value) - t(zero().value);
		}

		template<unsigned newBits>
		constexpr exponent_t<newBits> convert() const
		{
			using new_exponent_t = exponent_t<newBits>;
			if constexpr (bits == newBits)
				return *this;
			else
			{
				//constexpr int bits_exp_offset = OFFSET;
				constexpr int native_exp_offset = ((1 << newBits) / 2) - 1;

				if (value == 0)
					return exponent_t<newBits>(0);

				if constexpr (newBits >= bits)
				{
					if (*this == inf_or_nan())
						return new_exponent_t::inf_or_nan(); // inf/nan
				}

				const auto old_exponent_actual = actual_value();

				if constexpr (newBits < bits)
				{
					if (old_exponent_actual >= new_exponent_t::inf_or_nan().actual_value())
						return new_exponent_t::inf_or_nan();
					//if (old_exponent_actual < MIN)
					//	return
				}

				const int new_exponent = old_exponent_actual + native_exp_offset;

				return exponent_t<newBits>(new_exponent);
			}
		}

		template<unsigned otherBits>
		constexpr bool operator==(exponent_t<otherBits> other) const
		{
			if constexpr (bits == otherBits)
				return value == other.value;
			else if constexpr (bits > otherBits)
				return value == other.template convert<bits>().value;
			else if constexpr (bits < otherBits)
				return convert<otherBits>().value == other.value;
		}
		template<unsigned otherBits>
		constexpr bool operator!=(exponent_t<otherBits> other) const { return !operator==(other); }

		value_t value{};
	};

	template<unsigned MantissaBits_, unsigned ExponentBits_, bool SignBit_>
	class bit_float
	{
		using this_t = bit_float<MantissaBits_, ExponentBits_, SignBit_>;

		template<unsigned otherM, unsigned otherE, bool otherS> friend class bit_float;
		friend class std::numeric_limits<this_t>;

	public:
		static constexpr auto MantissaBits = MantissaBits_;
		static constexpr auto ExponentBits = ExponentBits_;
		static constexpr auto SignBit = SignBit_;
		static constexpr unsigned MANTISSA_OFFSET = 0;
		static constexpr unsigned EXPONENT_OFFSET = MantissaBits;
		static constexpr unsigned SIGN_OFFSET = MantissaBits + ExponentBits;
		static constexpr unsigned TOTAL_BITS = MantissaBits + ExponentBits + (SignBit ? 1 : 0);

	public:
		enum class bits_t : detail::bit_float_hpp::uint_for_bits_t<TOTAL_BITS>;
		using mantissa_t = mh::mantissa_t<MantissaBits>;
		using exponent_t = mh::exponent_t<ExponentBits>;
		using native_t = std::conditional_t<
			MantissaBits <= detail::bit_float_hpp::FLT_MNT_BITS && ExponentBits <= detail::bit_float_hpp::FLT_EXP_BITS,
			float, double>;

		using native_bitfloat_t = bit_float<
			MantissaBits <= detail::bit_float_hpp::FLT_MNT_BITS
				? detail::bit_float_hpp::FLT_MNT_BITS
				: detail::bit_float_hpp::DBL_MNT_BITS,
			ExponentBits <= detail::bit_float_hpp::FLT_EXP_BITS
				? detail::bit_float_hpp::FLT_EXP_BITS
				: detail::bit_float_hpp::DBL_EXP_BITS,
			true>;

	private:
		using bits_ut = std::underlying_type_t<bits_t>;

	public:
		bit_float() = delete;

		static constexpr mantissa_t bits_to_mantissa(bits_t bits)
		{
			return mantissa_t((bits_ut(bits) >> MANTISSA_OFFSET) & mantissa_t::MASK);
		}
		static constexpr exponent_t bits_to_exponent(bits_t bits)
		{
			return exponent_t((bits_ut(bits) >> EXPONENT_OFFSET) & exponent_t::MASK);
		}
		static constexpr bool bits_to_sign(bits_t bits)
		{
			if constexpr (SignBit)
				return bits_ut(bits) & (bits_ut(1) << SIGN_OFFSET);
			else
				return false;
		}

		static constexpr bool is_inf(bits_t bits)
		{
			return bits_to_exponent(bits).value == exponent_t::MASK &&
				bits_to_mantissa(bits).value == 0;
		}
		static constexpr bool is_nan(bits_t bits)
		{
			return bits_to_exponent(bits).value == exponent_t::MASK &&
				bits_to_mantissa(bits).value != 0;
		}

		static constexpr bits_t components_to_bits(mantissa_t mantissa, exponent_t exponent, [[maybe_unused]] bool sign_bit)
		{
			bits_ut ret{};
			ret |= bits_ut(mantissa.value & mantissa_t::MASK) << MANTISSA_OFFSET;
			ret |= bits_ut(exponent.value & exponent_t::MASK) << EXPONENT_OFFSET;

			if constexpr (SignBit)
				ret |= bits_ut(sign_bit) << SIGN_OFFSET;

			return bits_t(ret);
		}

		template<typename new_bit_float>
		static constexpr auto bits_to_bits(bits_t bits)
		{
			if constexpr (std::is_same_v<typename new_bit_float::bits_t, bits_t>)
			{
				return bits;
			}
			else
			{
				using new_bits_t = typename new_bit_float::bits_t;

				bool sign = false;
				if constexpr (SignBit) // Do we have a sign bit?
				{
					sign = bits_to_sign(bits);
					if constexpr (!new_bit_float::SignBit)
					{
						if (sign)
						{
							// We are negative, and the new bits don't have a sign bit.
							// Clamp to 0.
							return new_bits_t{};
						}
					}
				}

				const auto old_exponent = bits_to_exponent(bits);
				const auto new_exponent = old_exponent.template convert<new_bit_float::ExponentBits>();

				// Can we overflow to infinity from this conversion?
				constexpr bool needsOverflowCheck = new_bit_float::ExponentBits < ExponentBits;

				typename new_bit_float::mantissa_t new_mantissa{};
				if (!needsOverflowCheck ||
					new_exponent.value != new_exponent.MASK ||
					old_exponent.value == old_exponent.MASK)
				{
					// If the old exponent was full, copy the most significant mantissa bits (either NaN or inf)
					// If the new exponent was not full, this is is not a NaN or inf, and we're safe
					new_mantissa = bits_to_mantissa(bits).template convert<new_bit_float::MantissaBits>();
				}

				return new_bit_float::components_to_bits(new_mantissa, new_exponent, sign);
			}
		}

		template<unsigned newMantissaBits, unsigned newExponentBits, bool newSignBit>
		static constexpr auto bits_to_bits(bits_t bits)
		{
			return bits_to_bits<bit_float<newMantissaBits, newExponentBits, newSignBit>>(bits);
		}

		static constexpr native_t bits_to_native(bits_t bits)
		{
			static_assert(std::numeric_limits<native_t>::is_iec559);
			return detail::bit_float_hpp::bit_cast<native_t>(bits_to_bits<native_bitfloat_t>(bits));
		}

		static constexpr bits_t native_to_bits(native_t native)
		{
			static_assert(std::numeric_limits<native_t>::is_iec559);
			return native_bitfloat_t::template bits_to_bits<this_t>(
				detail::bit_float_hpp::bit_cast<typename native_bitfloat_t::bits_t>(native));
		}
	};

	using half_float = mh::bit_float<detail::bit_float_hpp::HLF_MNT_BITS, detail::bit_float_hpp::HLF_EXP_BITS, true>;
	using native_float = mh::bit_float<detail::bit_float_hpp::FLT_MNT_BITS, detail::bit_float_hpp::FLT_EXP_BITS, true>;
	using native_double = mh::bit_float<detail::bit_float_hpp::DBL_MNT_BITS, detail::bit_float_hpp::DBL_EXP_BITS, true>;
}

namespace std
{
	template<unsigned MantissaBits, unsigned ExponentBits, bool SignBit>
	class numeric_limits<mh::bit_float<MantissaBits, ExponentBits, SignBit>> final
	{
		using type = mh::bit_float<MantissaBits, ExponentBits, SignBit>;
		using bits_t = typename type::bits_t;

		static constexpr auto LOG10 = 0.30102999566398119521;

		static constexpr auto ceil(float val)
		{
			return val == intmax_t(val) ? val : intmax_t(val) + 1;
		}

	public:
		static constexpr bool is_specialized = true;
		static constexpr bool is_signed = SignBit;
		static constexpr bool is_integer = false;
		static constexpr bool is_exact = false;
		static constexpr bool has_infinity = true;
		static constexpr bool has_quiet_NaN = true;
		static constexpr bool has_signaling_NaN = false;
		static constexpr bool has_denorm = std::numeric_limits<typename type::native_t>::has_denorm;
		static constexpr bool has_denorm_loss = std::numeric_limits<typename type::native_t>::has_denorm_loss;
		static constexpr auto round_style = std::float_round_style::round_toward_zero; // We always truncate the mantissa

		static constexpr bool is_iec559 = std::is_same_v<type, mh::half_float> ||
			std::is_same_v<type, mh::native_float> ||
			std::is_same_v<type, mh::native_double>;
		static constexpr bool is_bounded = true;
		static constexpr bool is_modulo = false;

		static constexpr int digits = MantissaBits + 1;
		static constexpr int digits10 = (digits - 1) * LOG10;
		static constexpr int max_digits10 = ceil(digits * LOG10 + 1);

		static constexpr int radix = 2;

		static constexpr int max_exponent = ((1 << ExponentBits) / 2);
		static constexpr int min_exponent = 3 - max_exponent;

		static constexpr bool traps = false;
		static constexpr bool tinyness_before = std::numeric_limits<typename type::native_t>::tinyness_before;

		static constexpr bits_t min() noexcept
		{
			return type::components_to_bits(typename type::mantissa_t(0), typename type::exponent_t(1), false);
		}
		static constexpr bits_t max() noexcept
		{
			return type::components_to_bits(type::mantissa_t::max(), type::exponent_t::max(), false);
		}
	};
}

template<typename CharT, typename Traits, unsigned Bits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, mh::exponent_t<Bits> exponent)
{
	return os << +exponent.value;
}

template<typename CharT, typename Traits, unsigned Bits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, mh::mantissa_t<Bits> mantissa)
{
	return os << +mantissa.value;
}

template<typename CharT, typename Traits, unsigned MantissaBits, unsigned ExponentBits, bool SignBit>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
	typename mh::bit_float<MantissaBits, ExponentBits, SignBit>::bits_t bits)
{
	return os << mh::bit_float<MantissaBits, ExponentBits, SignBit>::bits_to_native(bits);
}
