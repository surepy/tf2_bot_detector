#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

#ifdef MH_COMPILE_LIBRARY
#include "mh/memory/buffer.hpp"
#else
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

MH_COMPILE_LIBRARY_INLINE mh::buffer::buffer() noexcept = default;

MH_COMPILE_LIBRARY_INLINE mh::buffer::buffer(buffer&& other) noexcept :
	m_Data(std::exchange(other.m_Data, nullptr)),
	m_Size(std::exchange(other.m_Size, 0))
{
}

MH_COMPILE_LIBRARY_INLINE mh::buffer::buffer(const buffer& other) :
	buffer(other.data(), other.size())
{
}

MH_COMPILE_LIBRARY_INLINE mh::buffer::buffer(size_t initialSize)
{
	resize(initialSize);
}

MH_COMPILE_LIBRARY_INLINE mh::buffer::buffer(const std::byte* ptr, size_t bytes) :
	buffer(bytes)
{
	std::memcpy(data(), ptr, bytes);
}

MH_COMPILE_LIBRARY_INLINE mh::buffer::~buffer() noexcept
{
	clear();
}

MH_COMPILE_LIBRARY_INLINE void mh::buffer::resize(size_t newSize)
{
	void* newPtr = std::realloc(m_Data, newSize);
	if (!newPtr)
		throw std::runtime_error("Failed to realloc");

	m_Data = reinterpret_cast<std::byte*>(newPtr);
	m_Size = newSize;
}

MH_COMPILE_LIBRARY_INLINE bool mh::buffer::reserve(size_t minSize)
{
	if (size() < minSize)
	{
		resize(minSize + minSize / 2);
		return true;
	}

	return false;
}

#if ((__cpp_lib_three_way_comparison >= 201907) || defined(_MSC_VER)) && (__cpp_impl_three_way_comparison >= 201907)
MH_COMPILE_LIBRARY_INLINE std::strong_ordering mh::buffer::operator<=>(const mh::buffer& other) const
{
	if (auto result = m_Size <=> other.m_Size; std::is_neq(result))
		return result;

	const auto result = memcmp(data(), other.data(), size());
	if (result < 0)
		return std::strong_ordering::less;
	else if (result > 0)
		return std::strong_ordering::greater;
	else
		return std::strong_ordering::equal;
}
#endif

MH_COMPILE_LIBRARY_INLINE void mh::buffer::clear() noexcept
{
	std::free(m_Data);
	m_Data = nullptr;
	m_Size = 0;
}
