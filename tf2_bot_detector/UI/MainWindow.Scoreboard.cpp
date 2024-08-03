#include "MainWindow.h"
#include "Config/Settings.h"
#include "Platform/Platform.h"
#include "UI/ImGui_TF2BotDetector.h"
#include "BaseTextures.h"
#include "GameData/IPlayer.h"
#include "Networking/SteamAPI.h"
#include "Networking/SteamHistoryAPI.h"
#include "Networking/LogsTFAPI.h"
#include "TextureManager.h"
#include "GenericErrors.h"

#include "Networking/HTTPHelpers.h"

#include <Util/ScopeGuards.h>
#include <Util/StorageHelper.h>
#include <mh/math/interpolation.hpp>
#include <mh/text/fmtstr.hpp>
#include <mh/text/formatters/error_code.hpp>

#include <string_view>

#include "nlohmann/json.hpp"

enum ScoreboardColumnID
{
	ScoreboardColumnID_ID,
	ScoreboardColumnID_Name,
	ScoreboardColumnID_Kills,
	ScoreboardColumnID_Death,
	ScoreboardColumnID_Time,
	ScoreboardColumnID_Ping,
	ScoreboardColumnID_SteamID
};


using namespace std::chrono_literals;
using namespace std::string_view_literals;
using namespace tf2_bot_detector;

void MainWindow::OnDrawScoreboard()
{
	const auto& style = ImGui::GetStyle();
	const auto currentFontScale = ImGui::GetCurrentFontScale();

	constexpr bool forceRecalc = false; // <- this is forever false, what is this for

	static constexpr float contentWidthMin = 500;
	float contentWidthMinOuter = contentWidthMin + style.WindowPadding.x * 2;

	// Horizontal scroller for color pickers
	OnDrawColorPickers("ScoreboardColorPickers",
		{
			{ "You", m_Settings.m_Theme.m_Colors.m_ScoreboardYouFG },
			{ "Connecting", m_Settings.m_Theme.m_Colors.m_ScoreboardConnectingFG },
			{ "Friendly", m_Settings.m_Theme.m_Colors.m_ScoreboardFriendlyTeamBG },
			{ "Enemy", m_Settings.m_Theme.m_Colors.m_ScoreboardEnemyTeamBG },
			{ "Cheater", m_Settings.m_Theme.m_Colors.m_ScoreboardCheaterBG },
			{ "Suspicious", m_Settings.m_Theme.m_Colors.m_ScoreboardSuspiciousBG },
			{ "Exploiter", m_Settings.m_Theme.m_Colors.m_ScoreboardExploiterBG },
			{ "Racist", m_Settings.m_Theme.m_Colors.m_ScoreboardRacistBG },
		});

	OnDrawTeamStats();

	const auto availableSpaceOuter = ImGui::GetContentRegionAvail();

	float extraScoreboardHeight = 0;
	if (availableSpaceOuter.x < contentWidthMinOuter)
	{
		ImGui::SetNextWindowContentSize(ImVec2{ contentWidthMin, -1 });
		extraScoreboardHeight += style.ScrollbarSize;
	}

	// kind of ugly number of 1.5, but applog takes too much space...
	const float scoreboardHeight = ImGui::GetContentRegionAvail().y / (m_Settings.m_UIState.m_MainWindow.m_AppLogEnabled ? 1.5f : 1);

	// this shit is ass
	if (ImGui::BeginChild("Scoreboard", { 0, scoreboardHeight + extraScoreboardHeight }, true, ImGuiWindowFlags_HorizontalScrollbar))
	{
		{
			// Real table begins here.
			ImGuiTableFlags flags = ImGuiTableFlags_NoSavedSettings |
				ImGuiTableFlags_Resizable |
				ImGuiTableFlags_Sortable |
				ImGuiTableFlags_SizingStretchSame;

			if (ImGui::BeginTable("ScoreboardTable", 7, flags)) {
				// Name header and column setup
				{
					ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoResize, .5f, ScoreboardColumnID_ID);
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 2.5f, ScoreboardColumnID_Name);
					ImGui::TableSetupColumn("Kills", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, .6f, ScoreboardColumnID_Kills);
					ImGui::TableSetupColumn("Deaths", ImGuiTableColumnFlags_None | ImGuiTableColumnFlags_PreferSortDescending, .6f, ScoreboardColumnID_Death);
					ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_None, .75f, ScoreboardColumnID_Time);
					ImGui::TableSetupColumn("Ping", ImGuiTableColumnFlags_None, .5f, ScoreboardColumnID_Ping);
					ImGui::TableSetupColumn("Steam ID", ImGuiTableColumnFlags_None, 1.5f, ScoreboardColumnID_SteamID);
				}

				//ImGui::TableSetupScrollFreeze();
				ImGui::TableHeadersRow();

				// this is mostly copied from m_Application->m_MainState->GeneratePlayerPrintData().
				// 100 = player compatiblilty.
				static IPlayer* printData[100] { nullptr };

				// we regenerate this array every frame, so.
				auto& world = m_Application->GetWorld();
				auto begin = std::begin(printData);
				auto end = std::end(printData);

				// fill the array with tables copied from world state.
				{
					auto* current = begin;

					// try to use lobby data and copy it over to playerprintdata.
					for (IPlayer& member : world.GetLobbyMembers()) {
						*current = &member;
						current++;
					}

					// We seem to have either an empty lobby or we're playing on a community server.
					// Just find the most recent status updates.
					if (current == begin) {
						for (IPlayer& playerData : world.GetPlayers()) {
							// i fail to understand what this check is for, but whatever.
							if (playerData.GetLastStatusUpdateTime() >= (world.GetLastStatusUpdateTime() - 15s))
							{
								*current = &playerData;
								current++;

								if (current >= end)
									break; // This might happen, but we're not in a lobby so everything has to be approximate
							}
						}
					}

					end = current;
				}


				ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();

				std::sort(begin, end, [&sort_specs](const IPlayer* lhs, const IPlayer* rhs) -> bool {
					assert(lhs);
					assert(rhs);

					// copied from CompareWithSortSpecs in imgui_demo.
					for (int n = 0; n < sort_specs->SpecsCount; n++)
					{
						const ImGuiTableColumnSortSpecs* sort_spec = &sort_specs->Specs[n];

						int delta = 0;


						switch (sort_spec->ColumnUserID)
						{
						case ScoreboardColumnID_ID: {
							auto luid = lhs->GetUserID();
							auto ruid = rhs->GetUserID();
							if (luid && ruid)
							{
								delta = luid.value() - ruid.value();
							}
							break;
						}
						case ScoreboardColumnID_Name: delta = strcmp(lhs->GetNameUnsafe().c_str(), rhs->GetNameUnsafe().c_str()); break;
						case ScoreboardColumnID_Kills: delta = lhs->GetScores().m_Kills - rhs->GetScores().m_Kills; break;
						case ScoreboardColumnID_Death: delta = lhs->GetScores().m_Deaths - rhs->GetScores().m_Deaths; break;
						case ScoreboardColumnID_Time: delta = lhs->GetConnectedTime() > rhs->GetConnectedTime() ? 1 : -1 ; break;
						case ScoreboardColumnID_Ping: delta = lhs->GetPing() - rhs->GetPing(); break;
						case ScoreboardColumnID_SteamID: delta = lhs->GetSteamID().ID > rhs->GetSteamID().ID ? 1 : -1 ; break;
						default: break;
						}

						// lhs is bigger than rhs.
						if (delta != 0)
							return sort_spec->SortDirection == ImGuiSortDirection_Ascending ? delta < 0 : delta > 0 ;
					}

					// we couldn't sort using the specs so its like whatever i guess
					return false;
				});

				// draw the sorted players
				std::for_each(begin, end, [&](IPlayer* p) -> void {
					OnDrawScoreboardRow(*p);
				});

				ImGui::EndTable();
			}
		}

		ImGui::EndChild();
	}
}

// src/dest terminology is mirroring that of opengl: dest color is the base color, src color is the color we are blending towards
static ImVec4 BlendColors(const std::array<float, 4>& dstColor, const std::array<float, 4>& srcColor, float scalar)
{
	assert(scalar >= 0);
	assert(scalar <= 1);

	scalar *= srcColor[3]; // src alpha

	std::array<float, 4> result;
	for (size_t i = 0; i < 4; i++)
		result[i] = (srcColor[i] * scalar) + (dstColor[i] * (1.0f - scalar));

	return ImVec4(result);
}

void MainWindow::OnDrawScoreboardRow(IPlayer& player)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);

	if (!m_Settings.m_LazyLoadAPIData)
		TryGetAvatarTexture(player);

	const auto& playerName = player.GetNameSafe();
	ImGuiDesktop::ScopeGuards::ID idScope((int)player.GetSteamID().Lower32);
	ImGuiDesktop::ScopeGuards::ID idScope2((int)player.GetSteamID().Upper32);

	ImGuiDesktop::ScopeGuards::StyleColor textColor;
	if (player.GetConnectionState() != PlayerStatusState::Active || player.GetNameSafe().empty())
		textColor = { ImGuiCol_Text, m_Settings.m_Theme.m_Colors.m_ScoreboardConnectingFG };
	else if (player.GetSteamID() == m_Settings.GetLocalSteamID())
		textColor = { ImGuiCol_Text, m_Settings.m_Theme.m_Colors.m_ScoreboardYouFG };

	mh::fmtstr<32> buf;
	if (!player.GetUserID().has_value())
		buf = "?";
	else
		buf.fmt("{}", player.GetUserID().value());

	bool shouldDrawPlayerTooltip = false;

	// Selectable
	const auto teamShareResult = m_Application->GetModLogic().GetTeamShareResult(player);
	const auto playerAttribs = m_Application->GetModLogic().GetPlayerAttributes(player);
	{
		ImVec4 bgColor = [&]() -> ImVec4
		{
			switch (teamShareResult)
			{
			case TeamShareResult::SameTeams:      return m_Settings.m_Theme.m_Colors.m_ScoreboardFriendlyTeamBG;
			case TeamShareResult::OppositeTeams:  return m_Settings.m_Theme.m_Colors.m_ScoreboardEnemyTeamBG;
			case TeamShareResult::Neither:        break;
			}

			switch (player.GetTeam())
			{
			case TFTeam::Red:   return ImVec4(1.0f, 0.5f, 0.5f, 0.5f);
			case TFTeam::Blue:  return ImVec4(0.5f, 0.5f, 1.0f, 0.5f);
			default: return ImVec4(0.5f, 0.5f, 0.5f, 0);
			}
		}();

		if (playerAttribs.Has(PlayerAttribute::Cheater))
			bgColor = BlendColors(bgColor.to_array(), m_Settings.m_Theme.m_Colors.m_ScoreboardCheaterBG, m_Application->TimeSine());
		else if (playerAttribs.Has(PlayerAttribute::Suspicious))
			bgColor = BlendColors(bgColor.to_array(), m_Settings.m_Theme.m_Colors.m_ScoreboardSuspiciousBG, m_Application->TimeSine());
		else if (playerAttribs.Has(PlayerAttribute::Exploiter))
			bgColor = BlendColors(bgColor.to_array(), m_Settings.m_Theme.m_Colors.m_ScoreboardExploiterBG, m_Application->TimeSine());
		else if (playerAttribs.Has(PlayerAttribute::Racist))
			bgColor = BlendColors(bgColor.to_array(), m_Settings.m_Theme.m_Colors.m_ScoreboardRacistBG, m_Application->TimeSine());

		ImGuiDesktop::ScopeGuards::StyleColor styleColorScope(ImGuiCol_Header, bgColor);

		bgColor.w = std::min(bgColor.w + 0.25f, 0.8f);
		ImGuiDesktop::ScopeGuards::StyleColor styleColorScopeHovered(ImGuiCol_HeaderHovered, bgColor);

		bgColor.w = std::min(bgColor.w + 0.5f, 1.0f);
		ImGuiDesktop::ScopeGuards::StyleColor styleColorScopeActive(ImGuiCol_HeaderActive, bgColor);
		ImGui::Selectable(buf.c_str(), true, ImGuiSelectableFlags_SpanAllColumns);

		shouldDrawPlayerTooltip = ImGui::IsItemHovered();

		ImGui::TableNextColumn();
	}

	OnDrawScoreboardContextMenu(player);

	// player names column
	{
		static constexpr bool DEBUG_ALWAYS_DRAW_ICONS = false;

		const auto columnEndX = ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x + ImGui::GetColumnWidth();

		if (!playerName.empty())
			ImGui::TextFmt(playerName);
		else if (const auto& summary = player.GetPlayerSummary(); summary && !summary->m_Nickname.empty())
			ImGui::TextFmt(summary->m_Nickname);
		else
			ImGui::TextFmt("<Unknown>");

		// If their steamcommunity name doesn't match their ingame name
		if (auto summary = player.GetPlayerSummary();
			summary && !playerName.empty() && summary->m_Nickname != playerName)
		{
			ImGui::SameLine();
			ImGui::TextFmt({ 1, 0, 0, 1 }, "({})", summary->m_Nickname);
		}

		// Move cursor pos up a few pixels if we have icons to draw
		struct IconDrawData
		{
			ImTextureID m_Texture;
			ImVec4 m_Color{ 1, 1, 1, 1 };
			std::string_view m_Tooltip;
		};
		std::vector<IconDrawData> icons;

		// Check their steam bans
		if (auto bans = player.GetPlayerBans())
		{
			// If they are VAC banned
			std::invoke([&]
				{
					if constexpr (!DEBUG_ALWAYS_DRAW_ICONS)
					{
						if (bans->m_VACBanCount <= 0)
							return;
					}

					auto icon = m_BaseTextures->GetVACShield_16();
					if (!icon)
						return;

					icons.push_back({ (ImTextureID)(intptr_t)icon->GetHandle(), { 1, 1, 1, 1 }, "VAC Banned" });
				});

			// If they are game banned
			std::invoke([&]
				{
					if constexpr (!DEBUG_ALWAYS_DRAW_ICONS)
					{
						if (bans->m_GameBanCount <= 0)
							return;
					}

					auto icon = m_BaseTextures->GetGameBanIcon_16();
					if (!icon)
						return;

					icons.push_back({ (ImTextureID)(intptr_t)icon->GetHandle(), { 1, 1, 1, 1 }, "Game Banned" });
				});
		}

		// If they are friends with us on Steam
		std::invoke([&]
			{
				if constexpr (!DEBUG_ALWAYS_DRAW_ICONS)
				{
					if (!player.IsFriend())
						return;
				}

				auto icon = m_BaseTextures->GetHeart_16();
				if (!icon)
					return;

				icons.push_back({ (ImTextureID)(intptr_t)icon->GetHandle(), { 1, 0, 0, 1 }, "Steam Friends" });
			});

		// They are SourceBanned
		if (auto sourceBans = player.GetPlayerSourceBanState()) {
			std::invoke([&]
				{
					if constexpr (!DEBUG_ALWAYS_DRAW_ICONS)
					{
						if (sourceBans->size() <= 0)
							return;
					}

					auto icon = m_BaseTextures->GetSourceBansIcon_16();
					if (!icon)
						return;

					icons.push_back({ (ImTextureID)(intptr_t)icon->GetHandle(), { 1, 1, 1, 1 }, "Has SourceBans Entries" });
				});
		}

		if (!icons.empty())
		{
			// We have at least one icon to draw
			ImGui::SameLine();

			// Move it up very slightly so it looks centered in these tiny rows
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2);

			const float iconSize = 16 * ImGui::GetCurrentFontScale();

			const auto spacing = ImGui::GetStyle().ItemSpacing.x;
			ImGui::SetCursorPosX(columnEndX - (iconSize + spacing) * icons.size());

			for (size_t i = 0; i < icons.size(); i++)
			{
				ImGui::Image(icons[i].m_Texture, { iconSize, iconSize }, { 0, 0 }, { 1, 1 }, icons[i].m_Color);

				ImGuiDesktop::ScopeGuards::TextColor color({ 1, 1, 1, 1 });
				if (ImGui::SetHoverTooltip(icons[i].m_Tooltip))
					shouldDrawPlayerTooltip = false;

				ImGui::SameLine(0, spacing);
			}
		}

		ImGui::TableNextColumn();
	}

	// Kills column
	{
		if (playerName.empty())
			ImGui::TextRightAligned("?");
		else
			ImGui::TextRightAlignedF("%u", player.GetScores().m_Kills);

		ImGui::TableNextColumn();
	}

	// Deaths column
	{
		if (playerName.empty())
			ImGui::TextRightAligned("?");
		else
			ImGui::TextRightAlignedF("%u", player.GetScores().m_Deaths);

		ImGui::TableNextColumn();
	}

	// Connected time column
	{
		if (playerName.empty())
		{
			ImGui::TextRightAligned("?");
		}
		else
		{
			ImGui::TextRightAlignedF("%u:%02u",
				std::chrono::duration_cast<std::chrono::minutes>(player.GetConnectedTime()).count(),
				std::chrono::duration_cast<std::chrono::seconds>(player.GetConnectedTime()).count() % 60);
		}

		ImGui::TableNextColumn();
	}

	// Ping column
	{
		if (playerName.empty())
			ImGui::TextRightAligned("?");
		else
			ImGui::TextRightAlignedF("%u", player.GetPing());

		ImGui::TableNextColumn();
	}

	// Steam ID column
	{
		const auto str = player.GetSteamID().str();
		ImGui::TextFmt(str);
	}

	if (shouldDrawPlayerTooltip)
		DrawPlayerTooltip(player, teamShareResult, playerAttribs);
}

void MainWindow::OnDrawScoreboardContextMenu(IPlayer& player)
{
	if (auto popupScope = ImGui::BeginPopupContextItemScope("PlayerContextMenu"))
	{
		ImGuiDesktop::ScopeGuards::StyleColor textColor(ImGuiCol_Text, { 1, 1, 1, 1 });

		const auto name = player.GetNameSafe().c_str();
		const auto steamID = player.GetSteamID();

		tf2_bot_detector::DrawPlayerContextCopyMenu(player.GetNameUnsafe().c_str(), steamID);

		tf2_bot_detector::DrawPlayerContextGoToMenu(m_Settings, steamID);

		const auto& world = m_Application->GetWorld();
		auto& modLogic = m_Application->GetModLogic();

		if (ImGui::BeginMenu("Votekick",
			(world.GetTeamShareResult(steamID, m_Settings.GetLocalSteamID()) == TeamShareResult::SameTeams) && world.FindUserID(steamID)))
		{
			if (ImGui::MenuItem("Cheating"))
				modLogic.InitiateVotekick(player, KickReason::Cheating);
			if (ImGui::MenuItem("Idle"))
				modLogic.InitiateVotekick(player, KickReason::Idle);
			if (ImGui::MenuItem("Other"))
				modLogic.InitiateVotekick(player, KickReason::Other);
			if (ImGui::MenuItem("Scamming"))
				modLogic.InitiateVotekick(player, KickReason::Scamming);

			ImGui::EndMenu();
		}

		ImGui::Separator();


		auto& data = player.GetOrCreateData<TF2BDApplication::PlayerExtraData>(player);

		DrawPlayerContextMarkMenu(player.GetSteamID(), player.GetNameSafe(), data.m_pendingReason);

		/*
		TODO: get moderatorlogic playerextradata and make a checkbox for m_IgnoreRules
		if (ImGui::BeginMenu("Misc"))
		{
			const auto extra_data = player.GetData<IModeratorLogic>();
			if (extra_data != nullptr) {

			}
			else {

				ImGui::Checkbox("");
			}

			ImGui::EndMenu();
		}*/
#ifdef _DEBUG
		ImGui::Separator();

		if (bool isRunning = m_Application->GetModLogic().IsUserRunningTool(player);
			ImGui::MenuItem("Is Running TFBD", nullptr, isRunning))
		{
			m_Application->GetModLogic().SetUserRunningTool(player, !isRunning);
		}
#endif
	}
}

void MainWindow::DrawPlayerTooltip(IPlayer& player)
{
	DrawPlayerTooltip(player, m_Application->m_WorldState->GetTeamShareResult(player), m_Application->GetModLogic().GetPlayerAttributes(player));
}

void MainWindow::DrawPlayerTooltip(IPlayer& player, TeamShareResult teamShareResult,
	const PlayerMarks& playerAttribs)
{
	ImGui::BeginTooltip();
	DrawPlayerTooltipBody(player, teamShareResult, playerAttribs);
	ImGui::EndTooltip();
}

static const ImVec4 COLOR_RED = { 1, 0, 0, 1 };
static const ImVec4 COLOR_YELLOW = { 1, 1, 0, 1 };
static const ImVec4 COLOR_GREEN = { 0, 1, 0, 1 };
static const ImVec4 COLOR_UNAVAILABLE = { 1, 1, 1, 0.5 };
static const ImVec4 COLOR_PRIVATE = COLOR_YELLOW;

namespace ImGui
{
	struct Span
	{
		using fmtstr_type = mh::fmtstr<256>;
		using string_type = std::string;

		template<typename... TArgs>
		Span(const ImVec4& color, const std::string_view& fmtStr, const TArgs&... args) :
			m_Color(color), m_Value(fmtstr_type(fmtStr, args...))
		{
		}

		template<typename... TArgs>
		Span(const std::string_view& fmtStr, const TArgs&... args) :
			m_Value(fmtstr_type(fmtStr, args...))
		{
		}

		Span(const std::string_view& text)
		{
			if (text.size() <= fmtstr_type::max_size())
				m_Value.emplace<fmtstr_type>(text);
			else
				m_Value.emplace<string_type>(text);
		}
		Span(const char* text) : Span(std::string_view(text)) {}

		std::string_view GetView() const
		{
			if (auto str = std::get_if<fmtstr_type>(&m_Value))
				return *str;
			else if (auto str = std::get_if<string_type>(&m_Value))
				return *str;

			LogError(MH_SOURCE_LOCATION_CURRENT(), "Unexpected variant index {}", m_Value.index());
			return __FUNCTION__ + std::string(": UNEXPECTED VARIANT INDEX");
		}

		std::optional<ImVec4> m_Color;
		std::variant<fmtstr_type, string_type> m_Value;
	};

	template<typename TIter>
	void TextSpan(TIter begin, TIter end)
	{
		if (begin == end)
			return;

		if (begin->m_Color.has_value())
			ImGui::TextFmt(*begin->m_Color, begin->GetView());
		else
			ImGui::TextFmt(begin->GetView());

		++begin;

		for (auto it = begin; it != end; ++it)
		{
			ImGui::SameLineNoPad();

			if (it->m_Color.has_value())
				ImGui::TextFmt(*it->m_Color, it->GetView());
			else
				ImGui::TextFmt(it->GetView());
		}
	}

	void TextSpan(const std::initializer_list<Span>& spans) { return TextSpan(spans.begin(), spans.end()); }
}

static void EnterAPIKeyText()
{
	ImGui::TextFmt(COLOR_UNAVAILABLE, "Enter Steam API key in Settings");
}

static void PrintPlayerSummary(const IPlayer& player)
{
	player.GetPlayerSummary()
		.or_else([&](std::error_condition err)
			{
				ImGui::TextFmt("Player Summary : ");
				ImGui::SameLineNoPad();

				if (err == std::errc::operation_in_progress)
					ImGui::PacifierText();
				else if (err == SteamAPI::ErrorCode::EmptyAPIKey)
					EnterAPIKeyText();
				else
					ImGui::TextFmt(COLOR_RED, "{}", err);
			})
		.map([&](const SteamAPI::PlayerSummary& summary)
			{
				using namespace SteamAPI;
				if (player.GetNameSafe() != summary.m_Nickname) {
					ImGui::TextFmt(COLOR_YELLOW, "    Steam Name : \"{}\"", summary.m_Nickname);
				}

				ImGui::TextFmt("     Real Name : ");
				ImGui::SameLineNoPad();
				if (summary.m_RealName.empty())
					ImGui::TextFmt(COLOR_UNAVAILABLE, "Not set");
				else
					ImGui::TextFmt("\"{}\"", summary.m_RealName);

				ImGui::TextFmt("    Vanity URL : ");
				ImGui::SameLineNoPad();
				if (auto vanity = summary.GetVanityURL(); !vanity.empty())
					ImGui::TextFmt("\"{}\"", vanity);
				else
					ImGui::TextFmt(COLOR_UNAVAILABLE, "Not set");

				ImGui::TextFmt("   Account Age : ");
				ImGui::SameLineNoPad();
				if (auto age = summary.GetAccountAge())
				{
					ImGui::TextFmt("{}", HumanDuration(*age));
				}
				else
				{
					ImGui::TextFmt(COLOR_PRIVATE, "Private");

					if (auto estimated = player.GetEstimatedAccountAge())
					{
						ImGui::SameLine();
						ImGui::TextFmt("(estimated {})", HumanDuration(*estimated));
					}
#ifdef _DEBUG
					else
					{
						ImGui::SameLine();
						ImGui::TextFmt("(estimated ???)");
					}
#endif
				}

				ImGui::TextFmt(" Profile State : ");
				ImGui::SameLineNoPad();
				switch (summary.m_Visibility)
				{
				case CommunityVisibilityState::Public:
					ImGui::TextFmt(COLOR_GREEN, "Public");
					break;
				case CommunityVisibilityState::FriendsOnly:
					ImGui::TextFmt(COLOR_PRIVATE, "Friends Only");
					break;
				case CommunityVisibilityState::Private:
					ImGui::TextFmt(COLOR_PRIVATE, "Private");
					break;
				default:
					ImGui::TextFmt(COLOR_RED, "Unknown ({})", int(summary.m_Visibility));
					break;
				}

				if (!summary.m_ProfileConfigured)
				{
					ImGui::SameLineNoPad();
					ImGui::TextFmt(", ");
					ImGui::SameLineNoPad();
					ImGui::TextFmt(COLOR_RED, "Not Configured");
				}
			});
}

static void PrintPlayerBans(const IPlayer& player)
{
	player.GetPlayerBans()
		.or_else([&](std::error_condition err)
			{
				ImGui::TextFmt("   Player Bans : ");
				ImGui::SameLineNoPad();

				if (err == std::errc::operation_in_progress)
					ImGui::PacifierText();
				else if (err == SteamAPI::ErrorCode::EmptyAPIKey)
					EnterAPIKeyText();
				else
					ImGui::TextFmt(COLOR_RED, "{}", err);
			})
		.map([&](const SteamAPI::PlayerBans& bans)
			{
				using namespace SteamAPI;
				if (bans.m_CommunityBanned)
					ImGui::TextSpan({ "SteamCommunity : ", { COLOR_RED, "Banned" } });

				if (bans.m_EconomyBan != PlayerEconomyBan::None)
				{
					ImGui::TextFmt("  Trade Status :");
					switch (bans.m_EconomyBan)
					{
					case PlayerEconomyBan::Probation:
						ImGui::TextFmt(COLOR_YELLOW, "Banned (Probation)");
						break;
					case PlayerEconomyBan::Banned:
						ImGui::TextFmt(COLOR_RED, "Banned");
						break;

					default:
					case PlayerEconomyBan::Unknown:
						ImGui::TextFmt(COLOR_RED, "Unknown");
						break;
					}
				}

				{
					const ImVec4 banColor = (bans.m_TimeSinceLastBan >= (24h * 365 * 7)) ? COLOR_YELLOW : COLOR_RED;
					if (bans.m_VACBanCount > 0)
						ImGui::TextFmt(banColor, "      VAC Bans : {}", bans.m_VACBanCount);
					if (bans.m_GameBanCount > 0)
						ImGui::TextFmt(banColor, "     Game Bans : {}", bans.m_GameBanCount);
					if (bans.m_VACBanCount || bans.m_GameBanCount)
						ImGui::TextFmt(banColor, "      Last Ban : {} ago", HumanDuration(bans.m_TimeSinceLastBan));
				}
			});
}

static void PrintPlayerSourceBans(const IPlayer& player)
{
	// TODO: notify feature disabled.

	player.GetPlayerSourceBanState()
		.or_else([&](std::error_condition err)
			{
				ImGui::TextFmt("    SourceBans : ");
				ImGui::SameLineNoPad();

				if (err == std::errc::operation_in_progress)
					ImGui::PacifierText();
				else if (err == tf2_bot_detector::ErrorCode::InternetConnectivityDisabled)
					ImGui::TextFmt(COLOR_UNAVAILABLE, "Enable in Settings");
				else
					ImGui::TextFmt(COLOR_RED, "{}", err);
			})
		.map([&](const SteamHistoryAPI::PlayerSourceBanState& banState)
			{
				// Ban Count:
				{
					const ImVec4 banColor = banState.size() > 0 ? COLOR_YELLOW : ImVec4{ 1, 1, 1, 1 };
					ImGui::TextFmt(banColor, "    SourceBans : {} bans", banState.size());
				}

				for (const auto& [server, ban] : banState) {
					ImGui::TextFmt("      {} (as {}) = ", server, std::quoted(ban.m_UserName));
					ImGui::SameLineNoPad();

					const ImVec4 banStateColor = ban.m_BanState >= tf2_bot_detector::SteamHistoryAPI::Current ? COLOR_YELLOW : ImVec4{ 1, 1, 1, 1 };
					ImGui::TextFmt(banStateColor, "{:v} ", mh::enum_fmt(ban.m_BanState));
					ImGui::SameLineNoPad();

					ImGui::TextFmt("[{} ago]", HumanDuration(tfbd_clock_t::now() - ban.m_BanTimestamp));

					ImGui::TextFmt("       - reason: {}", ban.m_BanReason);
				}
			});
}

static void PrintPlayerPlaytime(const IPlayer& player)
{
	ImGui::TextFmt("  TF2 Playtime : ");
	ImGui::SameLineNoPad();
	player.GetTF2Playtime()
		.or_else([&](std::error_condition err)
			{
				if (err == std::errc::operation_in_progress)
				{
					ImGui::PacifierText();
				}
				else if (err == SteamAPI::ErrorCode::InfoPrivate || err == SteamAPI::ErrorCode::GameNotOwned)
				{
					// The reason for the GameNotOwned check is that the API hides free games if you haven't played
					// them. So even if you can see other owned games, if you make your playtime private...
					// suddenly you don't own TF2 anymore, and it disappears from the owned games list.
					ImGui::TextFmt(COLOR_PRIVATE, "Private");
				}
				else if (err == SteamAPI::ErrorCode::EmptyAPIKey)
				{
					EnterAPIKeyText();
				}
				else
				{
					ImGui::TextFmt(COLOR_RED, "{}", err);
				}
			})
		.map([&](duration_t playtime)
			{
				const auto hours = std::chrono::duration_cast<std::chrono::hours>(playtime);
				ImGui::TextFmt("{} hours", hours.count());
			});
}

static void PrintPlayerLogsCount(const IPlayer& player)
{
	ImGui::TextFmt("       Logs.TF : ");
	ImGui::SameLineNoPad();

	player.GetLogsInfo()
		.or_else([&](std::error_condition err)
			{
				if (err == std::errc::operation_in_progress)
				{
					ImGui::PacifierText();
				}
				else
				{
					ImGui::TextFmt(COLOR_RED, "{}", err);
				}
			})
		.map([&](const LogsTFAPI::PlayerLogsInfo& info)
			{
				ImGui::TextFmt("{} logs", info.m_LogsCount);
			});
}

static void PrintPlayerMarkedFriendsCount(IPlayer& player, const IModeratorLogic& modLogic)
{
	ImGui::TextFmt("Friends Marked : ");
	ImGui::SameLineNoPad();

	player.GetFriendsInfo()
		.or_else([&](std::error_condition err)
			{
				if (err == std::errc::operation_in_progress)
				{
					ImGui::PacifierText();
				}
				else if (err == HTTPResponseCode::Unauthorized) {
					// should i just wait for getplayersummary?
					ImGui::TextFmt(COLOR_UNAVAILABLE, "Private", err);
				}
				else
				{
					ImGui::TextFmt(COLOR_RED, "{}", err);
				}
			})
		.map([&](const SteamAPI::PlayerFriends& info)
			{
				auto markedFriends = modLogic.GetMarkedFriendsCount(player);

				if (markedFriends.m_MarkedFriendsCountTotal == 0) {
					ImGui::TextFmt("0");
				}
				else {
					int markedRatio = int(markedFriends.m_MarkedFriendsCountTotal == 0 ? 0 : float(markedFriends.m_MarkedFriendsCountTotal) / markedFriends.m_FriendsCountTotal * 100);

					ImGui::TextFmt(COLOR_YELLOW, "{} ", markedFriends.m_MarkedFriendsCountTotal);
					ImGui::SameLineNoPad();
					ImGui::TextFmt("({}% Marked out of {})", markedRatio, markedFriends.m_FriendsCountTotal);

					for (const auto& [attrib, count] : markedFriends.m_MarkedFriendsCount) {
						if (count == 0) {
							continue;
						}

						ImGui::TextFmt(COLOR_YELLOW, "    {} ", count);
						ImGui::SameLineNoPad();
						ImGui::TextFmt(COLOR_YELLOW, "{:v}", mh::enum_fmt(attrib));
					}
				}
			});
}

static void PrintPlayerInventoryInfo(const IPlayer& player)
{
	ImGui::TextFmt("Inventory Size : ");
	ImGui::SameLineNoPad();

	player.GetInventoryInfo()
		.or_else([&](std::error_condition err)
			{
				if (err == std::errc::operation_in_progress)
				{
					ImGui::PacifierText();
				}
				else if (err == SteamAPI::ErrorCode::InfoPrivate)
				{
					ImGui::TextFmt(COLOR_PRIVATE, "Private");
				}
				else
				{
					ImGui::TextFmt(COLOR_RED, "{}", err);
				}
			})
		.map([&](const SteamAPI::PlayerInventoryInfo& info)
			{
				ImGui::TextFmt("{} items ({} slots)", info.m_Items, info.m_Slots);
			});
}

void MainWindow::DrawPlayerTooltipBody(IPlayer& player, TeamShareResult teamShareResult,
	const PlayerMarks& playerAttribs)
{
	ImGuiDesktop::ScopeGuards::StyleColor textColor(ImGuiCol_Text, { 1, 1, 1, 1 });

	/////////////////////
	// Draw the avatar //
	/////////////////////
	TryGetAvatarTexture(player)
		.or_else([&](std::error_condition ec)
			{
				if (ec != SteamAPI::ErrorCode::EmptyAPIKey)
					ImGui::Dummy({ 184, 184 });
			})
		.map([&](const std::shared_ptr<ITexture>& tex)
			{
				ImGui::Image((ImTextureID)(intptr_t)tex->GetHandle(), { 184, 184 });
			});

	////////////////////////////////
	// Fix up the cursor position //
	////////////////////////////////
	{
		const auto pos = ImGui::GetCursorPos();
		ImGui::SetItemAllowOverlap();
		ImGui::SameLine();
		ImGui::NewLine();
		ImGui::SetCursorPos(ImGui::GetCursorStartPos());
		ImGui::Indent(pos.y - ImGui::GetStyle().FramePadding.x);
	}

	///////////////////
	// Draw the text //
	///////////////////
	ImGui::TextFmt("  In-game Name : ");
	ImGui::SameLineNoPad();
	if (auto name = player.GetNameUnsafe(); !name.empty())
		ImGui::TextFmt("\"{}\"", name);
	else
		ImGui::TextFmt(COLOR_UNAVAILABLE, "Unknown");

	PrintPlayerSummary(player);
	PrintPlayerBans(player);
	PrintPlayerPlaytime(player);
	PrintPlayerLogsCount(player);
	PrintPlayerInventoryInfo(player);
	PrintPlayerMarkedFriendsCount(player, m_Application->GetModLogic());

#ifdef _DEBUG
	ImGui::TextFmt("   Active time : {}", HumanDuration(player.GetActiveTime()));
#endif

	if (teamShareResult != TeamShareResult::SameTeams)
	{
		auto kills = player.GetScores().m_LocalKills;
		auto deaths = player.GetScores().m_LocalDeaths;
		//ImGui::Text("Your Thirst: %1.0f%%", kills == 0 ? float(deaths) * 100 : float(deaths) / kills * 100);
		ImGui::TextFmt("  Their Thirst : {}%", int(deaths == 0 ? float(kills) * 100 : float(kills) / deaths * 100));
	}

	ImGui::NewLine();

	PrintPlayerSourceBans(player);

	if (playerAttribs)
	{
		ImGui::NewLine();
		ImGui::TextFmt("Player {} marked in playerlist(s):", player);

		for (auto& [fileName, data] : m_Application->GetModLogic().GetPlayerList()->FindPlayerData(player.GetSteamID())) {
			ImGui::Indent(18.0f);
			ImGui::TextFmt("- {} ({}):", std::quoted(fileName), data.GetAttributes());

			ImGui::Indent(27.0f);

			// as "name" at "time"
			// this will just not render if lastseen is null
			if (data.m_LastSeen.has_value()) {
				ImGui::Text("as");
				ImGui::SameLine();

				if (data.m_LastSeen->m_PlayerName.empty()) {
					ImGui::TextFmt({ 1, 1, 0, 1 }, "<unknown>");
				}
				else {
					ImGui::TextFmt("\"{}\"", data.m_LastSeen->m_PlayerName);
				}

				ImGui::SameLine();
				ImGui::Text("at");
				ImGui::SameLine();

				// NOTE: m_LastSeen is never updated by this application, so it's more like "first seen"
				// so treat it as first seen for now
				if (data.m_LastSeen->m_Time.time_since_epoch() == std::chrono::seconds(0)) {
					ImGui::TextFmt({ 1, 1, 0, 1 }, "<unknown>");
				}
				else {
					ImGui::TextFmt("{}", data.m_LastSeen->m_Time);
				}
			}

			ImGui::Text("reason:");
			ImGui::SameLine();

			if (data.m_Proof.empty()) {
				ImGui::TextFmt({ 1, 1, 0, 1 }, "<no reason given>");
			}
			else {
				for (const auto& p : data.m_Proof) {
					ImGui::TextFmt({ 0, 1, 1, 1 }, "{}", p.get<std::string>().c_str());
				}
			}
			ImGui::Unindent(27.0f);

			ImGui::Unindent(18.0f);
		}
	}
}

void MainWindow::DrawPlayerContextMarkMenu(const SteamID& steamid, const std::string& playername, std::string& reasons)
{
	if (ImGui::BeginMenu("Mark"))
	{
		IModeratorLogic& modLogic = m_Application->GetModLogic();

		ImGui::InputTextWithHint("", "Reason", &reasons, ImGuiInputTextFlags_CallbackAlways);

		for (int i = 0; i < (int)PlayerAttribute::COUNT; i++)
		{
			const auto attr = PlayerAttribute(i);
			const bool existingMarked = (bool)modLogic.HasPlayerAttributes(steamid, attr, AttributePersistence::Saved);

			if (ImGui::MenuItem(mh::fmtstr<512>("{:v}", mh::enum_fmt(attr)).c_str(), nullptr, existingMarked))
			{
				if (modLogic.SetPlayerAttribute(steamid, playername, attr, AttributePersistence::Saved, !existingMarked, reasons)) {
					Log("Manually marked {}{} {:v} | {}", playername, (existingMarked ? " NOT" : ""), mh::enum_fmt(attr), reasons);
					reasons = "";
				}
			}
		}

		ImGui::EndMenu();
	}
}

void MainWindow::OnDrawTeamStats()
{
	if (!m_Settings.m_UIState.m_MainWindow.m_TeamStatsEnabled)
		return;

	struct TeamStats
	{
		uint8_t m_PlayerCount = 0;
		uint8_t m_PlayerCountActive = 0;
		uint32_t m_Kills = 0;
		uint32_t m_Deaths = 0;
		uint32_t m_MarkedCount = 0;
		uint32_t m_MarkedCountActive = 0;

		duration_t m_AccountAgeSum{};
	};

	TeamStats statsArray[2]{};

	IWorldState& world = m_Application->GetWorld();

	for (const IPlayer& player : world.GetPlayers())
	{
		TeamStats* stats = nullptr;
		switch (world.GetTeamShareResult(player))
		{
		case TeamShareResult::SameTeams:
			stats = &statsArray[0];
			break;
		case TeamShareResult::OppositeTeams:
			stats = &statsArray[1];
			break;
		default:
			LogError("Unknown TeamShareResult");
			[[fallthrough]];
		case TeamShareResult::Neither:
			break;
		}

		if (!stats)
			continue;

		stats->m_PlayerCount++;

		if (player.GetConnectionState() == PlayerStatusState::Active) {
			stats->m_PlayerCountActive++;
		}

		bool foundAccountAge = false;
		if (auto summary = player.GetPlayerSummary())
		{
			if (auto age = summary->GetAccountAge())
			{
				stats->m_AccountAgeSum += *age;
				foundAccountAge = true;
			}
		}

		if (!foundAccountAge)
		{
			if (auto age = player.GetEstimatedAccountAge())
			{
				stats->m_AccountAgeSum += *age;
				foundAccountAge = true;
			}
		}

		stats->m_Kills += player.GetScores().m_Kills;
		stats->m_Deaths += player.GetScores().m_Deaths;

		if (!m_Application->GetModLogic().GetPlayerList()->GetPlayerAttributes(player).empty()) {
			stats->m_MarkedCount++;

			if (player.GetConnectionState() == PlayerStatusState::Active) {
				stats->m_MarkedCountActive++;
			}
		}
	}

	const auto& themeCols = m_Settings.m_Theme.m_Colors;
	auto friendlyBG = mh::lerp(themeCols.m_ScoreboardFriendlyTeamBG[3],
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg], (ImVec4)themeCols.m_ScoreboardFriendlyTeamBG);
	friendlyBG.w = 1;
	const auto enemyBG = themeCols.m_ScoreboardEnemyTeamBG;

	ImGuiDesktop::ScopeGuards::StyleColor progressBarFG(ImGuiCol_PlotHistogram, friendlyBG);
	ImGuiDesktop::ScopeGuards::StyleColor progressBarBG(ImGuiCol_FrameBg, enemyBG);

	const auto totalPlayers = statsArray[0].m_PlayerCount + statsArray[1].m_PlayerCount;
	const auto totalPlayersActive = statsArray[0].m_PlayerCountActive + statsArray[1].m_PlayerCountActive;

	if (totalPlayers > 0)
	{
		const float playersFraction = statsArray[0].m_PlayerCount / float(totalPlayers);

		ImGui::ProgressBar(playersFraction, { -FLT_MIN, 0 },
			mh::fmtstr<128>("Team Players: {} | {}", statsArray[0].m_PlayerCount, statsArray[1].m_PlayerCount).c_str());
	}

	if (const auto totalKills = statsArray[0].m_Kills + statsArray[1].m_Kills; totalKills > 0)
	{
		const float killsFraction = statsArray[0].m_Kills / float(totalKills);

		ImGui::ProgressBar(killsFraction, { -FLT_MIN, 0 },
			mh::fmtstr<128>("Team Kills: {} | {}", statsArray[0].m_Kills, statsArray[1].m_Kills).c_str());
	}

	if (totalPlayers > 0)
	{
		const auto totalMarked = statsArray[0].m_MarkedCount + statsArray[1].m_MarkedCount;
		const auto totalMarkedActive = statsArray[0].m_MarkedCountActive + statsArray[1].m_MarkedCountActive;
		const auto totalUnmarked = totalPlayers - totalMarked;
		const auto totalUnmarkedActive = totalPlayersActive - totalMarkedActive;
		const float markedRate = totalUnmarked / float(totalPlayers);

		// total MarkedVS

		// colors are kind of bad, whatever
		ImGuiDesktop::ScopeGuards::StyleColor markedFG(ImGuiCol_PlotHistogram, { 0.419607843f, 0.415686275f, 0.396078431f, 1 });
		ImGuiDesktop::ScopeGuards::StyleColor markedBG(ImGuiCol_FrameBg, themeCols.m_ScoreboardCheaterBG);

		if (totalMarkedActive > 0 && totalMarkedActive != totalMarked)
		{
			ImGui::ProgressBar(markedRate, { -FLT_MIN, 0 },
				mh::fmtstr<128>("MarkedVS (Total): {} ({}) vs {} ({})", totalUnmarked, totalUnmarkedActive, totalMarked, totalMarkedActive).c_str());
		}
		else
		{
			ImGui::ProgressBar(markedRate, { -FLT_MIN, 0 },
				mh::fmtstr<128>("MarkedVS (Total): {} vs {}", totalUnmarked, totalMarked).c_str());
		}

		if (statsArray[0].m_MarkedCount > 0)
		{
			// change colors for cooresponding teams 
			ImGuiDesktop::ScopeGuards::StyleColor teamMarkedFG(ImGuiCol_PlotHistogram, friendlyBG);

			const auto teamUnmarked = statsArray[0].m_PlayerCount - statsArray[0].m_MarkedCount;
			const auto teamUnmarkedActive = statsArray[0].m_PlayerCountActive - statsArray[0].m_MarkedCountActive;
			const float teamMarkedRate = teamUnmarked / static_cast<float>(statsArray[0].m_PlayerCount);

			mh::fmtstr<128> teamMessage;

			if (statsArray[0].m_MarkedCount != statsArray[0].m_MarkedCountActive)
			{
				teamMessage.fmt("MarkedVS (Team): {} ({}) vs {} ({})", teamUnmarked, teamUnmarkedActive, statsArray[0].m_MarkedCount, statsArray[0].m_MarkedCountActive);
			}
			else
			{
				teamMessage.fmt("MarkedVS (Team): {} vs {}", teamUnmarked, statsArray[0].m_MarkedCount);
			}

			ImGui::ProgressBar(teamMarkedRate, { -FLT_MIN, 0 }, teamMessage.c_str());
		}

		if (statsArray[1].m_MarkedCount > 0)
		{
			// change colors for cooresponding teams
			ImGuiDesktop::ScopeGuards::StyleColor enemyMarkedFG(ImGuiCol_PlotHistogram, enemyBG);

			const auto enemyUnmarked = statsArray[1].m_PlayerCount - statsArray[1].m_MarkedCount;
			const auto enemyUnmarkedActive = statsArray[1].m_PlayerCountActive - statsArray[1].m_MarkedCountActive;
			float enemyMarkedRate = enemyUnmarked / static_cast<float>(statsArray[1].m_PlayerCount);

			mh::fmtstr<128> enemyMessage;

			if (statsArray[1].m_MarkedCount != statsArray[1].m_MarkedCountActive)
			{
				enemyMessage.fmt("MarkedVS (Enemy): {} ({}) vs {} ({})", enemyUnmarked, enemyUnmarkedActive, statsArray[1].m_MarkedCount, statsArray[1].m_MarkedCountActive);
			}
			else
			{
				enemyMessage.fmt("MarkedVS (Enemy): {} vs {}", enemyUnmarked, statsArray[1].m_MarkedCount);
			}

			ImGui::ProgressBar(enemyMarkedRate, { -FLT_MIN, 0 }, enemyMessage.c_str());
		}
	}

#if 0
	if (const auto totalTime = statsArray[0].m_AccountAgeSum + statsArray[1].m_AccountAgeSum; totalTime.count() > 0)
	{
		const float timeFraction = float(statsArray[0].m_AccountAgeSum.count() / double(totalTime.count()));

		ImGui::ProgressBar(timeFraction, { -FLT_MIN, 0 },
			mh::fmtstr<128>("Team Account Age: {} | {}",
				HumanDuration(statsArray[0].m_AccountAgeSum), HumanDuration(statsArray[1].m_AccountAgeSum)).c_str());
	}
#endif
}
