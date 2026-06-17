#pragma once

#include <mh/source_location.hpp>
#include <ostream>
#ifdef __unix__
#include <signal.h>
#endif

namespace mh
{
	enum class ensure_trigger_result
	{
		ignore,
		debugger_break,
	};

	namespace detail::ensure_hpp
	{
#if __cpp_concepts >= 201907
		template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
		concept StreamInsertable = requires(T t)
		{
			{ std::declval<std::basic_ostream<CharT, Traits>>() << t };
		};
#endif

#ifndef MH_ERROR_ENSURE_HPP_DEBUGBREAK
#ifdef _MSC_VER
#define MH_ERROR_ENSURE_HPP_DEBUGBREAK()  __debugbreak()
#else
#ifndef SIGTRAP
#define SIGTRAP SIGABRT
#endif
#define MH_ERROR_ENSURE_HPP_DEBUGBREAK()  raise(SIGTRAP)
#endif
#endif

		struct ensure_info_base
		{
			const char* m_ExpressionText = nullptr;
			const char* m_Message = nullptr;
			mh::source_location m_Location{};
		};

		struct ensure_traits_default_base
		{
			virtual ~ensure_traits_default_base() = default;

		protected:
			virtual int get_max_details_width() const { return 7; }  // "message"
			MH_STUFF_API virtual void print_details_label(std::ostream& str, const char* label) const;

			MH_STUFF_API ensure_trigger_result trigger_generic(const ensure_info_base& info) const;

			virtual bool can_print_value() const = 0;
			virtual void print_value_generic(std::ostream& os, const ensure_info_base& info) const = 0;
		};
	}

	template<typename T>
	struct ensure_info : detail::ensure_hpp::ensure_info_base
	{
		const T& m_Value;
	};

	template<typename T>
	struct ensure_traits_default : detail::ensure_hpp::ensure_traits_default_base
	{
	public:
		virtual bool should_trigger(const T& expr) const { return !expr; }
		virtual ensure_trigger_result trigger(const ensure_info<T>& info) const
		{
			return trigger_generic(info);
		}

	protected:
		bool can_print_value() const override { return can_print_value_impl(); }
		virtual void print_value(std::ostream& str, const T& value) const
		{
			if constexpr (can_print_value_impl())
				str << value;
			else
				str << '{' << typeid(T).name() << '}';
		}

	private:
		static constexpr bool can_print_value_impl()
		{
#if __cpp_concepts >= 201907
			// I don't think doing this inline works in MSVC yet, shame
			return detail::ensure_hpp::StreamInsertable<T>;
#else
			return false;
#endif
		}

		void print_value_generic(std::ostream& os, const detail::ensure_hpp::ensure_info_base& info) const override final
		{
			print_value(os, static_cast<const ensure_info<T>&>(info).m_Value);
		}
	};

	template<typename T>
	struct ensure_traits final : ensure_traits_default<T>
	{
	};

#define mh_ensure_impl(expr, traitsType, msg) \
	([](auto&& value, MH_SOURCE_LOCATION_AUTO(location)) -> decltype(auto) \
	{ \
		using type_t = std::decay_t<decltype(value)>; \
		traitsType<type_t> traits; \
		if (traits.should_trigger(value)) [[unlikely]] \
		{ \
			::mh::ensure_info<type_t> info{ .m_Value = value }; \
			info.m_ExpressionText = #expr; \
			info.m_Message = msg; \
			info.m_Location = location; \
			\
			const auto result = traits.trigger(info); \
			if (result == ::mh::ensure_trigger_result::debugger_break) \
				MH_ERROR_ENSURE_HPP_DEBUGBREAK(); \
		} \
		return std::move(value); \
	}(expr, MH_SOURCE_LOCATION_CURRENT()))

#ifdef _DEBUG
#define mh_ensure_msg(expr, msg)  mh_ensure_impl(expr, ::mh::ensure_traits, msg)
#define mh_ensure(expr)           mh_ensure_msg(expr, nullptr)
#else
#define mh_ensure(expr)           (expr)
#define mh_ensure_msg(expr, msg)  (expr)
#endif
}

#ifndef MH_COMPILE_LIBRARY
#include "ensure.inl"
#endif
