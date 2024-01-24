#pragma once
#include "IPlayerListFileFormat.h"

namespace tf2_bot_detector::playerlist::formats {
	/// <summary>
	/// pazer's json format
	/// </summary>
	/// <see>
	/// https://raw.githubusercontent.com/PazerOP/tf2_bot_detector/master/schemas/v3/playerlist.schema.json
	/// </see>
	class JsonV3 : public IPlayerListFileFormat {
		std::filesystem::path m_File;
		std::string m_UpdateURL;
	public:
		JsonV3(const std::filesystem::path& path);

		bool isRemote() override;
		bool attemptLoad(std::vector<PlayerEntry>& out) override;

		/// <summary>
		/// looks like correct schema, we can attempt to call Load().
		/// </summary>
		/// <returns></returns>
		bool valid() override;
		bool save(const std::vector<PlayerEntry>& in) override;
	};
}
