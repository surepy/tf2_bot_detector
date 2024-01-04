#include "PlayerListManagementWindow.h"
#include "ITF2BotDetectorRenderer.h"
#include "ImGui_TF2BotDetector.h"
#include "Config/Settings.h"
#include "Util/PathUtils.h"
#include "Platform/Platform.h"

#include "Application.h"

#include <mh/algorithm/algorithm.hpp>
#include <mh/error/ensure.hpp>
#include <nlohmann/json.hpp>

namespace tf2_bot_detector
{

PlayerListManagementWindow::PlayerListManagementWindow() : m_Application(TF2BDApplication::GetApplication()) {

}

void PlayerListManagementWindow::Draw() {
	// if (!b_Open) { return; }
	if (!m_Application.GetMainState()) { return; }

	bool very_true = true;
	//ImGui::ShowDemoWindow(&very_true);

	const auto& official_list = m_Application.GetModLogic().GetPlayerList()->GetConfigFileGroup().m_OfficialList.try_get();

	if (ImGui::Begin("PlayerList Management", &bOpen)) {
		ImGui::Checkbox("so true", &very_true);

		// m_Application.GetWorld().GetPlayers()

		if (!official_list) {

			ImGui::Text("lists not loaded");
			ImGui::End();
			return;
		}

		ImGuiTabBarFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable |
			ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingStretchProp;

		static std::string search = "";
		ImGui::InputText("Player Search (doesn't work)", &search);

		

		if (ImGui::BeginTable("player_list", 7, flags)) {
			ImGui::TableSetupColumn("SteamID", ImGuiTableColumnFlags_DefaultSort);
			ImGui::TableSetupColumn("State");
			ImGui::TableSetupColumn("Data Source");
			ImGui::TableSetupColumn("Name"); // 3
			ImGui::TableSetupColumn("Reason"); // 4
			ImGui::TableSetupColumn("Last Seen"); // 5
			ImGui::TableSetupColumn("Actions");
			ImGui::TableHeadersRow();

			/*
			if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
				if (sort_specs->SpecsDirty)
				{
					MyItem::SortWithSortSpecs(sort_specs, items.Data, items.Size);
					sort_specs->SpecsDirty = false;
				}
			*/
			const auto* local_list = &m_Application.GetModLogic().GetPlayerList()->GetConfigFileGroup().GetLocalList();
			this->DrawFileEntries(local_list);

			// draw official file entries (this will always probably exist).
			if (official_list) {
				this->DrawFileEntries(official_list);
			}


			


			ImGui::EndTable();

		}

		ImGui::End();
	}
	//
}

void PlayerListManagementWindow::DrawFileEntries(const PlayerListJSON::PlayerListFile* file)
{
	for (const auto& [steam_id, player] : file->m_Players) {
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ImGui::TextFmt("{}", steam_id.str());

		ImGui::TableNextColumn();
		ImGui::TextFmt("{}", player.GetAttributes());
		// player.m_LastSeen.value().m_PlayerName

		ImGui::TableNextColumn();
		{
			std::string filename = file->GetName();

			if (filename.ends_with("/playerlist.json")) {
				ImGui::Text("Local List");
			}
			else {
				ImGui::Text(filename.c_str());
			}
		}

		ImGui::TableNextColumn();
		if (player.m_LastSeen.has_value()) {
			const auto& last_seen = player.m_LastSeen.value();

			if (!last_seen.m_PlayerName.empty()) {
				ImGui::Text(last_seen.m_PlayerName.c_str());
			}
			else {
				ImGui::TextFmt({ 1, 1, 0, 1 }, "Unknown");
			}

			ImGui::TableSetColumnIndex(5);
			ImGui::TextFmt("{}", last_seen.m_Time);
		}
		else {
			ImGui::TextFmt({ 1, 1, 0, 1 }, "Unknown");
			ImGui::TableSetColumnIndex(5);
			ImGui::TextFmt({ 1, 1, 0, 1 }, "Unknown");
		}

		ImGui::TableSetColumnIndex(4);
		if (!player.m_Proof.empty()) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 1), ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * 2));
			std::string id = "ProofChild_" + steam_id.str();

			if (ImGui::BeginChild(id.c_str(), ImVec2(-FLT_MIN, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX)) {
				for (auto proof : player.m_Proof) {
					ImGui::Text(proof.get<std::string>().c_str());
				}
			}
			ImGui::EndChild();
		}
		else {
			ImGui::TextFmt({ 1, 1, 1, 0.5 }, "None Given");
		}

		ImGui::TableSetColumnIndex(6);
		ImGui::Button("Edit");
		ImGui::Button("Move/Copy");


		//file->GetFileInfo().m_UpdateURL.empty();

		//official_list->m_Players.erase();

	}

}

};

