#pragma once

#include <algorithm>

namespace mh
{
	template<typename Container, typename Predicate, typename Factory>
	std::pair<bool, typename Container::iterator> find_or_add_if_factory(Container& container, const Predicate& pred, const Factory& factory)
	{
		if (auto found = std::find_if(container.begin(), container.end(), pred); found != container.end())
			return { false, found };

		return { true, container.insert(container.end(), factory()) };
	}

	template<typename Container, typename Predicate, typename T>
	inline std::pair<bool, typename Container::iterator> find_or_add_if(Container& container, const Predicate& pred, const T& value)
	{
		return find_or_add_if_factory(container, pred, [&value]() { return value; });
	}

	template<typename Container, typename T>
	inline std::pair<bool, typename Container::iterator> find_or_add(Container& container, const T& value)
	{
		return find_or_add_if(container, [&value](const auto& v) { return v == value; }, value);
	}

	template<typename Container>
	inline bool contains(const Container& container, const typename Container::value_type& value)
	{
		const auto end = std::end(container);
		return std::find(std::begin(container), end, value) != end;
	}

	template<typename Container>
	inline void erase(Container& container, const typename Container::value_type& value)
	{
		container.erase(std::remove(std::begin(container), std::end(container), value), std::end(container));
	}

	template<typename Container>
	inline void sort(Container& container)
	{
		return std::sort(std::begin(container), std::end(container));
	}
	template<typename Container, typename Compare>
	inline void sort(Container& container, Compare&& compare)
	{
		return std::sort(std::begin(container), std::end(container), std::move(compare));
	}
}
