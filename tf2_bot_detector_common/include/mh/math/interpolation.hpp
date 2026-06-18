#pragma once

#if !defined(MH_INTERPOLATION_DISABLE_UINT128) && !defined(MH_INTERPOLATION_UINT128_TYPE)

#if __has_include(<mh/math/uint128.hpp>)
	#include <mh/math/uint128.hpp>
	#define MH_INTERPOLATION_FOUND_MH_UINT128 1
#elif __has_include("mh/math/uint128.hpp")
	#include "mh/math/uint128.hpp"
	#define MH_INTERPOLATION_FOUND_MH_UINT128 1
#endif

#if MH_INTERPOLATION_FOUND_MH_UINT128
	#define MH_INTERPOLATION_UINT128_MUL64(a, b) mh::uint128::from_mul(a, b)
	#define MH_INTERPOLATION_UINT128_GET_LO64(u128) ((u128).template get_u64<0>())
#endif
#endif

#include <cmath>
#include <limits>
#include <numeric>
#include <type_traits>

#undef min
#undef max

namespace mh
{
	namespace detail::interpolation_hpp
	{
		template<typename T>
		constexpr T round(T in)
		{
			if constexpr (std::is_integral_v<T>)
			{
				return in;
			}
#if __cpp_lib_is_constant_evaluated >= 201811
			else if (!std::is_constant_evaluated())
			{
				return std::round(in);
			}
#endif
			else
			{
				if (in >= 0)
					return in + 0.5f;
				else
					return in - 0.5f;
			}
		}

		template<typename TIn, typename TOut>
		constexpr TOut clamp(TIn in, TOut out_min, TOut out_max)
		{
			if (in <= static_cast<TIn>(out_min))
				return out_min;
			if (in >= static_cast<TIn>(out_max))
				return out_max;

			if constexpr (std::is_floating_point_v<TIn> && std::is_integral_v<TOut>)
				in = round(in);

			return static_cast<TOut>(in);
		}

		template<typename T>
		constexpr bool will_overflow_mul(T a, T b)
		{
			if (a == 0)
				return false;

			return T(T(a * b) / a) != b;
		}

		template<typename T>
		constexpr bool will_overflow_add(T a, T b)
		{
			return T(a + b) < a;
		}


		template<typename T> static constexpr bool has_larger_version_v = std::is_same_v<std::make_unsigned_t<T>, uint8_t>
			|| std::is_same_v<std::make_unsigned_t<T>, uint16_t>
			|| std::is_same_v<std::make_unsigned_t<T>, uint32_t>;

		template<typename T>
		static constexpr auto larger_version_helper()
		{
			if constexpr (std::is_same_v<T, int8_t>)
				return int16_t{};
			else if constexpr (std::is_same_v<T, int16_t>)
				return int32_t{};
			else if constexpr (std::is_same_v<T, int32_t>)
				return int64_t{};
			else if constexpr (std::is_same_v<T, uint8_t>)
				return uint16_t{};
			else if constexpr (std::is_same_v<T, uint16_t>)
				return uint32_t{};
			else if constexpr (std::is_same_v<T, uint32_t>)
				return uint64_t{};
		}
		template<typename T> using larger_version_t = decltype(larger_version_helper<T>());
	}

	template<typename TIn, typename TOut>
	constexpr TOut lerp_slow(TIn in_01, TOut out_min, TOut out_max)
	{
		static_assert(std::is_floating_point_v<TIn>);
		return (out_min * (1 - in_01)) + (out_max * in_01);
	}

	template<typename TIn, typename TOut>
	constexpr TOut lerp(TIn in_01, TOut out_min, TOut out_max)
	{
		static_assert(std::is_floating_point_v<TIn>);
		return out_min + (out_max - out_min) * in_01;
	}

	template<typename TIn, typename TOut>
	constexpr TOut lerp_clamped(TIn in_01, TOut out_min, TOut out_max)
	{
		using ct = std::common_type_t<TIn, TOut>;

		return detail::interpolation_hpp::clamp(
			lerp<TIn, ct>(in_01, out_min, out_max),
			out_min, out_max);
	}

	template<typename TIn, typename TOut>
	constexpr TOut lerp_slow_clamped(TIn in_01, TOut out_min, TOut out_max)
	{
		using ct = std::common_type_t<TIn, TOut>;

		return detail::interpolation_hpp::clamp(
			lerp_slow<TIn, ct>(in_01, out_min, out_max),
			out_min, out_max);
	}

	template<typename TIn, typename TOut = float>
	constexpr TOut remap_to_01(TIn in, TIn in_min, TIn in_max)
	{
		static_assert(std::is_floating_point_v<TOut>);
		return TOut(in - in_min) / TOut(in_max - in_min);
	}

	template<typename TIn, typename TOut>
	constexpr TOut remap(TIn in, TIn in_min, TIn in_max, TOut out_min, TOut out_max)
	{
		assert(in_min != in_max);
		return lerp(remap_to_01(in, in_min, in_max), out_min, out_max);
	}

	template<typename TIn, typename TOut>
	constexpr TOut remap_clamped(TIn in, TIn in_min, TIn in_max, TOut out_min, TOut out_max)
	{
		return lerp_clamped(remap_to_01(in, in_min, in_max), out_min, out_max);
	}

	template<typename TSrc, typename TDest,
		TSrc src_min = std::numeric_limits<TSrc>::min(),
		TSrc src_max = std::numeric_limits<TSrc>::max(),
		TDest dest_min = std::numeric_limits<TDest>::min(),
		TDest dest_max = std::numeric_limits<TDest>::max()>
	inline constexpr TDest remap_static(TSrc value)
	{
		using namespace mh::detail::interpolation_hpp;

		static_assert(src_min < src_max);
		static_assert(dest_min < dest_max);

		if constexpr (src_min == dest_min && src_max == dest_max)
		{
			return value;
		}
		else if constexpr (std::is_integral_v<TSrc> && std::is_integral_v<TDest>)
		{
			// TODO: don't choose the integer-only path if it would result in an integer overflow
			using TSrcUnsigned = std::make_unsigned_t<TSrc>;
			using TDestUnsigned = std::make_unsigned_t<TDest>;

			constexpr TSrcUnsigned src_urange = TSrcUnsigned(TSrcUnsigned(src_max) - TSrcUnsigned(src_min));
			constexpr TDestUnsigned dest_urange = TDestUnsigned(TDestUnsigned(dest_max) - TDestUnsigned(dest_min));
			static_assert(src_urange > 0);
			static_assert(dest_urange > 0);

			constexpr auto gcd = std::gcd(dest_urange, src_urange);
			constexpr TDestUnsigned num = dest_urange / gcd;
			constexpr TSrcUnsigned den = src_urange / gcd;

			const TSrcUnsigned valueOffset = TSrcUnsigned(value) - TSrcUnsigned(src_min);

			using TCommon = std::common_type_t<TSrcUnsigned, TDestUnsigned>;

			// Calculate the whole part
			TDestUnsigned result;
			{
				constexpr TDestUnsigned wholeMultiplier = num / den;
				static_assert(!will_overflow_mul<TCommon>(src_urange, wholeMultiplier));
				result = valueOffset * wholeMultiplier;
			}

			if constexpr (den != 1) // If denominator is not 1, then we need rounding logic
			{
				// Calculate the fractional part
				constexpr TCommon fracMultiplier = (num % den);
				constexpr auto half_den = (den / 2) - 1;

				constexpr bool has_more_native_bits = has_larger_version_v<TCommon>;

#ifdef MH_INTERPOLATION_UINT128_MUL64
				constexpr bool needs_more_bits = will_overflow_mul<TCommon>(src_urange, fracMultiplier);
				if constexpr (needs_more_bits && !has_more_native_bits)
				{
					// Perform using 128-bit unsigned integers
					const auto frac = MH_INTERPOLATION_UINT128_MUL64(valueOffset, fracMultiplier);
					const auto u128_value = (frac + half_den) / den;
					result += MH_INTERPOLATION_UINT128_GET_LO64(u128_value);
				}
				else
#endif
				{
					using frac_t = std::conditional_t<has_more_native_bits, larger_version_t<TCommon>, TCommon>;
					static_assert(std::numeric_limits<TCommon>::max() <= std::numeric_limits<frac_t>::max());
					static_assert(!will_overflow_mul<frac_t>(src_urange, fracMultiplier));
					constexpr frac_t frac_max = src_urange * fracMultiplier;

					const frac_t frac = frac_t(valueOffset) * frac_t(fracMultiplier);

					constexpr bool will_fast_div_round_overflow = will_overflow_add<frac_t>(frac_max, half_den);
					constexpr bool can_fast_div_round =
						!will_fast_div_round_overflow || has_larger_version_v<frac_t>;

					if constexpr (can_fast_div_round)
					{
						using round_t = std::conditional_t<will_fast_div_round_overflow, larger_version_t<frac_t>, frac_t>;

						// No conditional/mod required because we doubled the number of
						// bits we have to work with, and at most we'll have (max * max) + max
						static_assert(!will_overflow_add<round_t>(frac_max, half_den));
						result += (round_t(frac) + round_t(half_den)) / den;
					}
					else
					{
						const auto remainder = frac % den;
						result += frac / den;

						constexpr auto half_den_mod = (den / 2) + (den % 2);
						if (remainder > half_den_mod)
							result += 1;
					}
				}
			}

			// Shift range from [0, (dest_max - dest_min)] to [dest_min, dest_max]
			result += dest_min;

			return result;
		}
		else
		{
			if constexpr (src_min == 0 && src_max == 1)
			{
				return lerp_clamped(value, dest_min, dest_max);
			}
			else if constexpr (dest_min == 0 && dest_max == 1)
			{
				return remap_to_01(value, src_min, src_max);
			}
			else
			{
				return remap_clamped(
					value, src_min, src_max, dest_min, dest_max);
			}
		}
	}
}
