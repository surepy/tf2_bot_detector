#pragma once

#if (__cpp_concepts >= 201907) || (defined(_MSC_VER) && (__cpp_concepts >= 201811))

#include <exception>
#include <type_traits>

namespace mh
{
	namespace detail::scope_exit_hpp
	{
#if __cpp_lib_remove_cvref >= 201711
		template<typename T> using remove_cvref_t = std::remove_cvref_t<T>;
#else
		template<typename T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

		template<typename EF, typename Fn, typename TSelf>
		concept ConstructibleFunc =
			!std::is_same_v<remove_cvref_t<Fn>, TSelf> &&
			std::is_constructible_v<EF, Fn>;

		template<typename EF, typename Fn, typename TSelf>
		concept ConstructibleForwardFunc =
			ConstructibleFunc<EF, Fn, TSelf> &&
			!std::is_lvalue_reference_v<Fn> &&
			std::is_nothrow_constructible_v<EF, Fn>;

		template<typename EF, typename Fn, typename TSelf>
		concept ConstructibleCopyFunc =
			ConstructibleFunc<EF, Fn, TSelf> &&
			!ConstructibleForwardFunc<EF, Fn, TSelf>;

		template<typename EF>
		concept MoveFunc =
			std::is_nothrow_move_constructible_v<EF> ||
			std::is_copy_constructible_v<EF>;

		template<typename EF>
		concept MoveForwardFunc = MoveFunc<EF> && std::is_nothrow_move_constructible_v<EF>;

		template<typename EF>
		concept MoveCopyFunc = MoveFunc<EF> && !MoveForwardFunc<EF>;

		struct scope_traits_all final
		{
			constexpr bool operator()() const { return true; }
		};
		struct scope_traits_fail final
		{
			bool operator()() const { return std::uncaught_exceptions() > 0; }
		};
		struct scope_traits_success final
		{
			bool operator()() const { return std::uncaught_exceptions() <= 0; }
		};

		// https://en.cppreference.com/w/cpp/experimental/scope_exit
		template<typename EF, typename Traits>
		class [[nodiscard]] scope_exit_base
		{
			using self_type = scope_exit_base<EF, Traits>;

		public:
			template<typename Fn>
			explicit scope_exit_base(Fn&& fn, bool enabled = true)
				noexcept(std::is_nothrow_constructible_v<EF, Fn> || std::is_nothrow_constructible_v<EF, Fn&>)
				requires ConstructibleForwardFunc<EF, Fn, self_type> :
				m_Func(std::forward<Fn>(fn)),
				m_Active(enabled)
			{
			}
			template<typename Fn>
			explicit scope_exit_base(Fn&& fn, bool enabled = true)
				noexcept(std::is_nothrow_move_constructible_v<EF> || std::is_nothrow_copy_constructible_v<EF>)
				requires ConstructibleCopyFunc<EF, Fn, self_type> :
				m_Func(fn),
				m_Active(enabled)
			{
			}

			scope_exit_base(self_type&& other)
				noexcept(std::is_nothrow_move_constructible_v<EF> || std::is_nothrow_copy_constructible_v<EF>)
				requires MoveForwardFunc<EF> :
				m_Active(other.m_Active),
				m_Func(std::forward<EF>(other.m_Func))
			{
				other.release();
			}
			scope_exit_base(self_type&& other)
				noexcept(std::is_nothrow_move_constructible_v<EF> || std::is_nothrow_copy_constructible_v<EF>)
				requires MoveCopyFunc<EF> :
				m_Active(other.m_Active),
				m_Func(std::forward<EF>(other.m_Func))
			{
				other.release();
			}

			scope_exit_base(const self_type&) = delete;

			~scope_exit_base() noexcept
			{
				if (m_Active && m_Traits())
					m_Func();
			}

			scope_exit_base& operator=(const self_type&) = delete;
			scope_exit_base& operator=(self_type&&) = delete;

			operator bool() const { return m_Active; }

			void release() noexcept
			{
				m_Active = false;
			}

		private:
			bool m_Active = true;

#if __has_cpp_attribute(no_unique_address)
			[[no_unique_address]]
#endif
			EF m_Func;

#if __has_cpp_attribute(no_unique_address)
			[[no_unique_address]]
#endif
			Traits m_Traits;
		};
	}

	template<typename EF>
	struct [[nodiscard]] scope_exit : detail::scope_exit_hpp::scope_exit_base<EF, detail::scope_exit_hpp::scope_traits_all>
	{
		using scope_exit_base = detail::scope_exit_hpp::scope_exit_base<EF, detail::scope_exit_hpp::scope_traits_all>;
		using scope_exit_base::scope_exit_base;
	};
	template<typename EF> scope_exit(EF) -> scope_exit<EF>;

	template<typename EF>
	struct [[nodiscard]] scope_fail : detail::scope_exit_hpp::scope_exit_base<EF, detail::scope_exit_hpp::scope_traits_fail>
	{
		using scope_exit_base = detail::scope_exit_hpp::scope_exit_base<EF, detail::scope_exit_hpp::scope_traits_fail>;
		using scope_exit_base::scope_exit_base;
	};
	template<typename EF> scope_fail(EF) -> scope_fail<EF>;

	template<typename EF>
	struct [[nodiscard]] scope_success : detail::scope_exit_hpp::scope_exit_base<EF, detail::scope_exit_hpp::scope_traits_success>
	{
		using scope_exit_base = detail::scope_exit_hpp::scope_exit_base<EF, detail::scope_exit_hpp::scope_traits_success>;
		using scope_exit_base::scope_exit_base;
	};
	template<typename EF> scope_success(EF) -> scope_success<EF>;
}

#endif
