#pragma once

#if defined(__INTELLISENSE__) || __has_include(<coroutine>)
#include <coroutine>
#define MH_COROUTINES_SUPPORTED 1
namespace mh::detail
{
	namespace coro = std;
}
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define MH_COROUTINES_SUPPORTED 1
namespace mh::detail
{
	namespace coro = std::experimental;
}
#endif
