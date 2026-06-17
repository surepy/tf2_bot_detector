#pragma once

#if (__cpp_concepts >= 201907) || (_MSC_VER >= 1928)

#include <string_view>
#include <tuple>

namespace mh
{
	template<typename TStruct, typename TMember>
	struct struct_member_info
	{
		using struct_type = TStruct;
		using value_type = TMember;

		std::string_view name;
		TStruct& obj;
		TMember& value;
		TMember TStruct::* pointer;
		size_t offset;
	};

	namespace detail::reflection::struct_hpp
	{
		template<typename T> struct impl;

		template<typename T>
		struct make_dependent
		{
			using type = T;
		};

		template<typename T>
		concept HasBaseTypes = requires
		{
			typename T::mh_struct_reflect_bases_t;
		};

		template<typename TObj, typename TFunc, size_t I>
		constexpr void handle_bases_impl(TObj&& obj, TFunc&& func)
		{
			using tuple_t = typename TObj::mh_struct_reflect_bases_t;
			if constexpr (I >= std::tuple_size_v<tuple_t>)
			{
				return;
			}
			else
			{
				using current_t = std::tuple_element_t<I, tuple_t>;
				impl<current_t>::for_each(std::forward<current_t>(obj), std::forward<TFunc>(func));
				return handle_bases_impl<TObj, TFunc, I + 1>(std::forward<TObj>(obj), std::forward<TFunc>(func));
			}
		}

		template<HasBaseTypes TObj, typename TFunc>
		constexpr void handle_bases(typename make_dependent<TObj>::type&& obj, TFunc&& func, int)
		{
			handle_bases_impl<TObj, TFunc, 0>(std::forward<TObj>(obj), std::forward<TFunc>(func));
		}

		template<typename TObj, typename TFunc>
		constexpr auto handle_bases(TObj&&, TFunc&&, void*)
		{
			// No mh_struct_reflect_bases_t, so nothing to do.
		}

		template<typename T, typename TFunc>
		constexpr void for_each_member(typename detail::reflection::struct_hpp::make_dependent<T>::type&& obj, TFunc&& func)
		{
			detail::reflection::struct_hpp::impl<std::decay_t<T>>::for_each(std::forward<T>(obj), std::forward<TFunc>(func));
		}
	}

	template<typename T, typename TFunc>
	constexpr void for_each_member(T&& obj, TFunc&& func)
	{
		detail::reflection::struct_hpp::for_each_member<T, TFunc>(std::forward<T>(obj), std::forward<TFunc>(func));
	}
}

#define MH_STRUCT_REFLECT_BASES(...) \
	using mh_struct_reflect_bases_t = std::tuple<__VA_ARGS__>;

#define MH_STRUCT_REFLECT_BEGIN(...) \
	template<> \
	struct mh::detail::reflection::struct_hpp::impl<__VA_ARGS__> \
	{ \
		using value_type = __VA_ARGS__; \
		\
		template<typename TObj, typename TFunc, typename = std::enable_if_t<std::is_same_v<std::decay_t<TObj>, value_type>>> \
		static constexpr void for_each(TObj&& obj, TFunc&& func) \
		{ \
			mh::detail::reflection::struct_hpp::handle_bases<TObj, TFunc>(std::forward<value_type>(obj), std::forward<TFunc>(func), 0);

#define MH_STRUCT_REFLECT_MEMBER(member) \
			func(mh::struct_member_info<value_type, decltype(obj.member)>{ #member, obj, obj.member, &value_type::member, offsetof(value_type, member) });

#define MH_STRUCT_REFLECT_END() \
		} \
	};

#endif
