#pragma once

#include <cstdint>

namespace tf2_bot_detector
{
	enum class TFTeam : uint8_t
	{
		Unknown,

		Spectator,
		Red,
		Blue,
	};

	using UserID_t = uint16_t;

#if MH_FORMATTER == MH_FORMATTER_FMTLIB && FMT_VERSION >= 90000
	//auto format_as(TFTeam f) { return fmt::underlying(f); }
#endif
}
