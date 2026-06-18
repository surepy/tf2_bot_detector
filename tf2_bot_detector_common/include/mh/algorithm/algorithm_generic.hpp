#pragma once

#include <type_traits>

namespace mh
{
	template<typename TContainer, typename TFunc>
	inline constexpr void for_each_multimap_group(TContainer&& container, TFunc&& func)
	{
		using container_type = std::decay_t<TContainer>;
		using key_type = typename container_type::key_type;
		using range_t = decltype(container.equal_range(std::declval<key_type>()));

		range_t range;
		for (auto it = container.begin(); it != container.end(); it = range.second)
		{
			range = container.equal_range(it->first);

			func(it->first, range.first, range.second);
		}
	}
}
