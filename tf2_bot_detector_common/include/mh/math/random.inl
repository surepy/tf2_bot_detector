#ifdef MH_COMPILE_LIBRARY
#include "random.hpp"
#else
#define MH_COMPILE_LIBRARY_INLINE inline
#endif

#include <random>

#undef min
#undef max

namespace mh
{
	namespace detail::random_hpp
	{
		inline static thread_local std::mt19937 s_Engine = []
		{
			std::mt19937 retVal;

			std::random_device device;
			retVal.seed(device());

			return retVal;
		}();
	}

	template<typename T>
	MH_COMPILE_LIBRARY_INLINE T get_random(T min_inclusive, T max_inclusive)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			std::uniform_real_distribution<T> dist(min_inclusive, std::nextafter(max_inclusive, std::numeric_limits<T>::max()));
			return dist(detail::random_hpp::s_Engine);
		}
		else if constexpr (std::is_same_v<T, int8_t>)
		{
			return static_cast<int8_t>(get_random<int16_t>(min_inclusive, max_inclusive));
		}
		else if constexpr (std::is_same_v<T, uint8_t>)
		{
			return static_cast<uint8_t>(get_random<uint16_t>(min_inclusive, max_inclusive));
		}
		else
		{
			std::uniform_int_distribution<T> dist(min_inclusive, max_inclusive);
			return dist(detail::random_hpp::s_Engine);
		}
	}

#ifdef MH_COMPILE_LIBRARY

#define GET_RANDOM_DECL(type) template type get_random(type, type)

	GET_RANDOM_DECL(float);
	GET_RANDOM_DECL(double);

	GET_RANDOM_DECL(int8_t);
	GET_RANDOM_DECL(uint8_t);
	GET_RANDOM_DECL(int16_t);
	GET_RANDOM_DECL(uint16_t);
	GET_RANDOM_DECL(int32_t);
	GET_RANDOM_DECL(uint32_t);
	GET_RANDOM_DECL(int64_t);
	GET_RANDOM_DECL(uint64_t);

#undef GET_RANDOM_DECL
#endif
}
