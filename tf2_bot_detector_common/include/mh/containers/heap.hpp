#pragma once

#include <algorithm>
#include <vector>

namespace mh
{
	namespace detail::heap_hpp
	{
		template<typename T>
		struct less
		{
			constexpr bool operator()(const T& lhs, const T& rhs) const
			{
				return lhs < rhs;
			}
		};
	}

	template<typename T, typename TComparator = detail::heap_hpp::less<T>, typename TContainer = std::vector<T>>
	class heap final
	{
	public:
		explicit heap(TComparator comparator = TComparator{}) :
			m_Comparator(std::move(comparator))
		{
		}

		explicit heap(std::initializer_list<T> values, TComparator comparator = TComparator{}) :
			m_Container(values.begin(), values.end()), m_Comparator(std::move(comparator))
		{
		}

		void push(const T& value)
		{
			m_Container.push_back(value);
			std::push_heap(m_Container.begin(), m_Container.end(), m_Comparator);
		}
		void push(T&& value)
		{
			m_Container.push_back(std::move(value));
			std::push_heap(m_Container.begin(), m_Container.end(), m_Comparator);
		}

		void pop()
		{
			std::pop_heap(m_Container.begin(), m_Container.end(), m_Comparator);
			m_Container.pop_back();
		}

		[[nodiscard]] bool empty() const { return m_Container.empty(); }
		[[nodiscard]] size_t size() const { return m_Container.size(); }

		T& front() { return m_Container.front(); }
		const T& front() const { return m_Container.front(); }

		friend bool operator==(const heap& lhs, const heap& rhs) { return lhs.m_Container == rhs.m_Container; }

	private:
		TContainer m_Container;
#if __has_cpp_attribute(no_unique_address)
		[[no_unique_address]]
#endif
		TComparator m_Comparator{};
	};
}
