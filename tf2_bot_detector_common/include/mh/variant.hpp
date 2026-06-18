#pragma once

#include <type_traits>
#include <variant>

#ifdef __cpp_consteval
#define MH_CONSTEVAL consteval
#else
#define MH_CONSTEVAL constexpr
#endif

namespace mh
{
	namespace detail::variant
	{
		constexpr size_t INVALID_TYPE_INDEX = size_t(-1);

		template<size_t Index, typename TFind, typename T, typename... TOthers>
		MH_CONSTEVAL size_t find_type_index()
		{
			if constexpr (std::is_same_v<TFind, T>)
			{
				if constexpr (sizeof...(TOthers) > 0)
				{
					static_assert(find_type_index<0, TFind, TOthers...>() == INVALID_TYPE_INDEX,
						"Multiple indices have the same type");
				}

				return Index;
			}
			else if (sizeof...(TOthers) < 1)
			{
				return INVALID_TYPE_INDEX;
			}
			else
			{
				return find_type_index<Index + 1, TFind, TOthers...>();
			}
		}

		template<typename TFind, typename TVariant, size_t Index = 0>
		MH_CONSTEVAL size_t find_variant_index_noerror()
		{
			if constexpr (Index >= std::variant_size_v<TVariant>)
			{
				return INVALID_TYPE_INDEX;
			}
			else if constexpr (std::is_same_v<TFind, std::variant_alternative_t<Index, TVariant>>)
			{
				static_assert(find_variant_index_noerror<TFind, TVariant, Index + 1>() == INVALID_TYPE_INDEX,
					"Multiple indicies have the same type");

				return Index;
			}
			else
			{
				return find_variant_index_noerror<TFind, TVariant, Index + 1>();
			}
		}

		template<typename TFind, typename TVariant, size_t Index = 0>
		MH_CONSTEVAL size_t find_variant_index()
		{
			constexpr size_t index = find_variant_index_noerror<TFind, TVariant, Index>();
			static_assert(index != INVALID_TYPE_INDEX, "Type not found in variant");
			return index;
		}
	}

	template<typename TFind, typename... Types>
	MH_CONSTEVAL size_t variant_type_index(const std::variant<Types...>&)
	{
		return detail::variant::find_type_index<0, TFind, Types...>();
	}

	template<typename TVariant, typename TFind> constexpr size_t variant_type_index_v =
		detail::variant::find_variant_index<TFind, TVariant>();
}

#undef MH_CONSTEVAL
