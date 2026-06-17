#pragma once

#if __has_include(<version>)
#include <version>
#endif

#if ((__cpp_lib_three_way_comparison >= 201907) || defined(_MSC_VER)) && (__cpp_impl_three_way_comparison >= 201907)
#include <compare>
#endif
#include <cstddef>

#ifndef MH_STUFF_API
#define MH_STUFF_API
#endif

namespace mh
{
	class buffer final
	{
	public:
		MH_STUFF_API buffer() noexcept;
		MH_STUFF_API buffer(buffer&& other) noexcept;
		MH_STUFF_API explicit buffer(const buffer& other);
		MH_STUFF_API explicit buffer(size_t initialSize);
		MH_STUFF_API buffer(const std::byte* ptr, size_t bytes);
		MH_STUFF_API ~buffer() noexcept;

		MH_STUFF_API void resize(size_t newSize);
		MH_STUFF_API bool reserve(size_t minSize);

#if ((__cpp_lib_three_way_comparison >= 201907) || defined(_MSC_VER)) && (__cpp_impl_three_way_comparison >= 201907)
		MH_STUFF_API std::strong_ordering operator<=>(const mh::buffer& other) const;
#endif

		MH_STUFF_API void clear() noexcept;
		size_t size() const { return m_Size; }

		std::byte* data() { return m_Data; }
		const std::byte* data() const { return m_Data; }

	private:
		std::byte* m_Data = nullptr;
		size_t m_Size = 0;
	};
}

#ifndef MH_COMPILE_LIBRARY
#include "buffer.inl"
#endif
