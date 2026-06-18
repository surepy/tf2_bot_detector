#pragma once

#include "coroutine_include.hpp"

#ifdef MH_COROUTINES_SUPPORTED

#include <cassert>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <variant>
#include <utility>

namespace mh
{
	template<typename T> class generator;

	namespace detail::generator_hpp
	{
		template<typename T>
		struct promise
		{
		public:
			using value_type = std::remove_reference_t<T>;
			using reference = std::conditional_t<std::is_reference_v<T>, T, T&>;
			using const_reference = const std::remove_reference_t<T>&;
			using pointer = value_type*;

		private:
			using io_type = std::conditional_t<std::is_rvalue_reference_v<T>, T, const T&>;
			using storage_type = std::conditional_t<std::is_rvalue_reference_v<T>, value_type*, const value_type*>;

		public:

			constexpr detail::coro::suspend_always initial_suspend() const noexcept { return {}; }
			constexpr detail::coro::suspend_always final_suspend() const noexcept { return {}; }

			constexpr generator<T> get_return_object();

			constexpr detail::coro::suspend_always yield_value(io_type value)
			{
				m_State = std::addressof(value);
				return {};
			}

			void unhandled_exception()
			{
				m_State = std::current_exception();
			}

			void return_void()
			{
				// Nothing to do
			}

			const_reference& value() const
			{
				switch (m_State.index())
				{
				case 0:
					throw std::runtime_error("value() called on promise with no state");
				case 1:
					return *std::get<1>(m_State);
				case 2:
					std::rethrow_exception(std::get<2>(m_State));
				default:
					throw std::logic_error("invalid promise state");
				}
			}

			io_type value()
			{
				return const_cast<reference>(const_cast<const promise<T>*>(this)->value());
			}

			void rethrow_if_exception() const
			{
				if (auto ptr = std::get_if<std::exception_ptr>(&m_State))
					std::rethrow_exception(*ptr);
			}

		private:
			std::variant<std::monostate, storage_type, std::exception_ptr> m_State;
		};

		struct iterator_end {};

		template<typename T>
		struct iterator
		{
			using iterator_category = std::input_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = typename promise<T>::value_type;
			using reference = typename promise<T>::reference;
			using pointer = typename promise<T>::pointer;

			using self_type = iterator<T>;
			using handle_type = detail::coro::coroutine_handle<promise<T>>;

			iterator(handle_type handle) : m_Handle(std::move(handle)) {}

			constexpr bool done() const { return m_Handle.done(); }

			self_type& operator++() { m_Handle.resume(); return *this; }

			auto operator*() const -> decltype(auto) { return m_Handle.promise().value(); }

		private:
			handle_type m_Handle;
		};

		template<typename T> constexpr bool operator==(const iterator<T>& it, const iterator_end&)
		{
			return it.done();
		}
		template<typename T> constexpr bool operator==(const iterator_end&, const iterator<T>& it)
		{
			return it.done();
		}
		template<typename T> constexpr bool operator!=(const iterator<T>& it, const iterator_end& end) { return !(it == end); }
		template<typename T> constexpr bool operator!=(const iterator_end& end, const iterator<T>& it) { return !(end == it); }
	}

	template<typename T>
	class [[nodiscard]] generator
	{
	public:
		using promise_type = detail::generator_hpp::promise<T>;
		using coroutine_type = detail::coro::coroutine_handle<promise_type>;

		generator(coroutine_type handle) : m_Handle(std::move(handle)) {}

		generator(generator&& other) noexcept : m_Handle(std::exchange(other.m_Handle, nullptr)) {}
		generator& operator=(generator&& other) noexcept
		{
			assert(std::addressof(other) != this);
			if (m_Handle)
				m_Handle.destroy();

			m_Handle = std::exchange(other.m_Handle, nullptr);
			return *this;
		}

		~generator()
		{
			if (m_Handle)
				m_Handle.destroy();
		}

		detail::generator_hpp::iterator<T> begin()
		{
			if (m_Handle)
			{
				m_Handle.resume();
				m_Handle.promise().rethrow_if_exception();
			}

			return { m_Handle };
		}
		detail::generator_hpp::iterator_end end() { return {}; }

	private:
		coroutine_type m_Handle;
	};

	template<typename T>
	inline constexpr generator<T> detail::generator_hpp::promise<T>::get_return_object()
	{
		return { coro::coroutine_handle<typename generator<T>::promise_type>::from_promise(*this) };
	}

	template<typename TIter>
	static generator<typename std::iterator_traits<TIter>::value_type> make_generator(TIter begin, TIter end)
	{
		for (auto it = begin; it != end; ++it)
			co_yield *it;
	}
}

#endif
