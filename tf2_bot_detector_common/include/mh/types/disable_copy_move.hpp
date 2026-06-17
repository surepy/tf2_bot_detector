#pragma once

namespace mh
{
	struct disable_copy
	{
		constexpr disable_copy() = default;
		disable_copy(const disable_copy&) = delete;
		disable_copy& operator=(const disable_copy&) = delete;
	};

	struct disable_move
	{
		constexpr disable_move() = default;
		disable_move(disable_move&&) = delete;
		disable_move& operator=(disable_move&&) = delete;
	};

	struct disable_copy_move : disable_copy, disable_move {};
}
