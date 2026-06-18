#ifdef MH_COMPILE_LIBRARY
#include "exception_details.hpp"
#endif

#ifndef MH_COMPILE_LIBRARY_INLINE
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

#include <cassert>
#include <map>
#include <mutex>
#include <typeindex>

namespace mh::detail::exception_to_string_hpp
{
	struct handler_list
	{
		std::map<std::type_index, const mh::exception_details_handler*> m_Handlers;
		std::mutex m_Mutex;
	};

	MH_COMPILE_LIBRARY_INLINE handler_list& get_handler_list()
	{
		static handler_list s_Value;
		return s_Value;
	}
}

MH_COMPILE_LIBRARY_INLINE mh::exception_details::handler::handler(const std::type_info* type) noexcept :
	m_Type(type)
{
}

MH_COMPILE_LIBRARY_INLINE mh::exception_details::handler::handler(handler&& other) noexcept :
	m_Type(std::exchange(other.m_Type, nullptr))
{
	assert(this != &other);
}

MH_COMPILE_LIBRARY_INLINE mh::exception_details::handler& mh::exception_details::handler::operator=(handler&& other) noexcept
{
	assert(this != &other);

	release();

	m_Type = std::exchange(other.m_Type, nullptr);

	return *this;
}

MH_COMPILE_LIBRARY_INLINE mh::exception_details::handler::~handler()
{
	release();
}

MH_COMPILE_LIBRARY_INLINE void mh::exception_details::handler::release()
{
	if (m_Type)
	{
		auto& handlers = detail::exception_to_string_hpp::get_handler_list();
		std::lock_guard lock(handlers.m_Mutex);
		[[maybe_unused]] const auto removedCount = handlers.m_Handlers.erase(*m_Type);
		assert(removedCount == 1);
		m_Type = nullptr;
	}
}

MH_COMPILE_LIBRARY_INLINE const char* mh::exception_details::type_name() const noexcept
{
	return m_Type ? m_Type->name() : "<NULL>";
}

MH_COMPILE_LIBRARY_INLINE mh::exception_details::handler mh::exception_details::add_handler(
	const std::type_info& type, const exception_details_handler& handler)
{
	auto& handlers = detail::exception_to_string_hpp::get_handler_list();
	std::lock_guard lock(handlers.m_Mutex);
	auto result = handlers.m_Handlers.insert({ type, &handler });

	mh::exception_details::handler test(nullptr);

	return mh::exception_details::handler(result.second ? &type : nullptr);
}

MH_COMPILE_LIBRARY_INLINE mh::exception_details::exception_details(const std::exception_ptr& ep)
{
	try
	{
		try
		{
			std::rethrow_exception(ep);
		}
		catch (const std::nested_exception& nested)
		{
			m_Nested = nested.nested_ptr();
			throw;
		}
	}
	catch (const std::exception& e)
	{
		m_Type = &typeid(e);
		m_Message = e.what();
	}
	catch (const char* e)
	{
		m_Type = &typeid(e);
		m_Message = e ? e : "<NULL>";
	}
	catch (const std::string& e)
	{
		m_Type = &typeid(e);
		m_Message = e;
	}
	catch (...)
	{
		auto& handlers = detail::exception_to_string_hpp::get_handler_list();
		std::lock_guard lock(handlers.m_Mutex);

		bool handled = false;
		for (const auto& handler : handlers.m_Handlers)
		{
			handled = handler.second->try_handle(ep, *this);
			if (handled)
				break;
		}

		if (!handled)
		{
			m_Type = nullptr;
			m_Message = "<unknown>";
		}
	}
}
