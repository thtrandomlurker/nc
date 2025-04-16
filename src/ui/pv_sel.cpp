#include <array>
#include <stdint.h>
#include <stdio.h>
#include <hooks.h>
#include <nc_log.h>
#include <diva.h>
#include <nc_state.h>
#include <db.h>
#include "pv_sel.h"

constexpr uint32_t AetSelSetID   = 14010050;
constexpr uint32_t AetSelSceneID = 14010051;
constexpr uint32_t SprSelSetID   = 14020050;
constexpr const char* StyleNamesInternal[4] = { "arcade", "console", "mixed", "max" };

constexpr const char* SetChangeSoundName = "se_ft_music_selector_set_change_01";
constexpr const char* SortFilterSoundName = "se_ft_music_selector_sortfilter_change_01";

namespace pvsel
{
	std::string GSWindow::GetModePrefix() { return game::IsFutureToneMode() ? "ps4_" : "nsw_"; }
	std::string GSWindow::GetLanguageSuffix()
	{
		if (GetGameLocale() >= GameLocale_Max)
			return "_en";

		const char* suffixes[GameLocale_Max] = { "_jp", "_en", "_zh", "_tw", "_kr", "_fr", "_it", "_de", "_sp" };
		return suffixes[GetGameLocale()];
	}

	std::string GSWindow::GetBaseLayerName() { return GetModePrefix() + "game_style_win"; }
	std::string GSWindow::GetBaseTextLayerName() { return GetModePrefix() + "game_style_txt" + GetLanguageSuffix(); }
	std::string GSWindow::GetStyleTextLayerName(int32_t style)
	{
		return GetModePrefix() + "game_style_" + StyleNamesInternal[style] + GetLanguageSuffix();
	}

	void GSWindow::Reset()
	{
		option_count = 0;
		memset(options, GameStyle_Max, MaxOptionCount);
		aet::Stop(&base_win);
		aet::Stop(&base_txt);
		aet::Stop(&style_txt_buffer[0]);
		aet::Stop(&style_txt_buffer[1]);
		style_txt_index = 0;
		selected_index = 0;
		previous_option = GameStyle_Max;
	}

	void GSWindow::SetOptions(const uint8_t* opts, int32_t count, bool clean)
	{
		if (count < 0 || count > MaxOptionCount || !opts)
			return;

		int32_t new_index = 0;
		for (int32_t i = 0; i < count; i++)
			if (opts[i] == options[selected_index])
				new_index = i;

		selected_index = new_index;
		memcpy(options, opts, count);
		option_count = count;
		dirty = !clean;
	}

	void GSWindow::ForceSetOption(int32_t opt)
	{
		uint8_t option = opt;
		SetOptions(&option, 1);
	}

	bool GSWindow::TrySetSelectedOption(int32_t opt)
	{
		for (int32_t i = 0; i < option_count; i++)
		{
			if (options[i] == opt)
			{
				selected_index = i;
				dirty = true;
				MakeAetDirty();
				return true;
			}
		}

		return false;
	}

	void GSWindow::UpdateAet()
	{
		if (base_win == 0)
		{
			base_win = aet::PlayLayer(
				AetSelSceneID, 
				10,
				0x10000,
				GetBaseLayerName().c_str(),
				nullptr,
				nullptr,
				nullptr
			);
		}

		if (base_txt == 0)
		{
			base_txt = aet::PlayLayer(
				AetSelSceneID,
				10,
				0x10000,
				GetBaseTextLayerName().c_str(),
				nullptr,
				nullptr,
				nullptr
			);
		}

		if (options[selected_index] != previous_option)
		{
			AetHandle* txt = &style_txt_buffer[style_txt_index];
			if (*txt != 0)
			{
				aet::Stop(txt);
				*txt = aet::PlayLayer(
					AetSelSceneID,
					10,
					GetStyleTextLayerName(previous_option).c_str(),
					AetAction_OutOnce
				);

				if (++style_txt_index > 1)
					style_txt_index = 0;
			}

			aet::Stop(&style_txt_buffer[style_txt_index]);
			style_txt_buffer[style_txt_index] = aet::PlayLayer(
				AetSelSceneID,
				10,
				GetStyleTextLayerName(options[selected_index]).c_str(),
				AetAction_InLoop
			);
		}

		diva::vec3 offset;
		offset.x = game::IsFutureToneMode() && !IsToggleable() ? -18.0f : 0.0f;
		offset.y = 0.0f;
		offset.z = 0.0f;
		aet::SetPosition(style_txt_buffer[0], &offset);
		aet::SetPosition(style_txt_buffer[1], &offset);

		previous_option = options[selected_index];
	}

	void GSWindow::SetVisible(bool visible)
	{
		hidden = !visible;
		aet::SetVisible(base_win, visible);
		aet::SetVisible(base_txt, visible);
		aet::SetVisible(style_txt_buffer[0], visible);
		aet::SetVisible(style_txt_buffer[1], visible);
	}

	bool GSWindow::Ctrl()
	{
		if (game::IsCustomizeSelTaskReady())
		{
			SetVisible(false);
			return false;
		}

		diva::InputState* is = diva::GetInputState(0);
		if (IsToggleable() && (is->IsButtonTapped(92) || is->IsButtonTapped(13)))
		{
			if (++selected_index >= option_count)
				selected_index = 0;

			dirty = true;
			MakeAetDirty();
			sound::PlaySoundEffect(1, is_sort_mode ? SortFilterSoundName : SetChangeSoundName, 1.0f);
		}

		SetVisible(true);

		if (aet_dirty)
		{
			aet_dirty = false;
			UpdateAet();
		}

		// NOTE: Reset dirty flag and return it's value
		bool dirty = this->dirty;
		this->dirty = false;
		return dirty;
	}

	void GSWindow::Disp() const
	{
		if (base_win == 0 || hidden || !IsToggleable())
			return;

		const uint32_t key_sprite_ids_nsw[6] = {
			3203571017, // Xbox
			2779278485, // DualShock 4
			2152960672, // JoyCon
			312905805,  // Steam Deck
			257587290,  // Keyboard
			0
		};

		const uint32_t key_sprite_ids_ps4[6] = {
			585831999,  // Xbox
			1474623112, // DualShock 4
			3007666668, // JoyCon
			1166812452, // Steam Deck
			2835797874, // Keyboard
			0
		};

		const uint32_t* sprites = game::IsFutureToneMode() ? key_sprite_ids_ps4 : key_sprite_ids_nsw;

		AetComposition comp;
		aet::GetComposition(&comp, base_win);

		if (auto it = comp.find("p_key_icon_c"); it != comp.end())
		{
			SprArgs args = { };
			args.id = sprites[diva::GetInputState(0)->GetDevice()];
			args.trans = it->second.position;
			args.scale.x = it->second.matrix.row0.x;
			args.scale.y = it->second.matrix.row1.y;
			args.scale.z = 1.0f;
			args.resolution_mode_sprite = 14;
			args.resolution_mode_screen = 14;
			memset(args.color, 0xFF, 4);
			args.attr = 0x400000; // SPR_ATTR_CTR_CC
			args.priority = 11;
			spr::DrawSprite(&args);
		}
	}
}

namespace pvsel
{
	static bool assets_loaded = false;


	/*
	bool ui_visible    = true;
	int32_t info_style_index  = 0;
	int32_t sel_info_style[2] = { 0, 0 };
	int32_t sel_info_win      = 0;
	int32_t sel_info_txt      = 0;

	int32_t cur_pv_id      = -1;
	int32_t cur_diff       = -1;
	int32_t cur_edition    = -1;
	int32_t prev_sel_mode  = GameStyle_Max;
	int32_t selected_index = 0;
	int32_t mode_count     = 0;
	int32_t modes[3]       = { GameStyle_Max, GameStyle_Max, GameStyle_Max };

	static bool IsModeChangeable() { return mode_count > 1; }
	static void PopulateModes(int32_t pv, int32_t diff, int32_t ed)
	{
		mode_count = 0;

		if (const auto* entry = db::FindDifficultyEntry(pv, diff, ed); entry != nullptr)
		{
			for (const db::ChartEntry& chart : entry->charts)
			{
				if (chart.style == GameStyle_Max)
					continue;

				modes[mode_count++] = chart.style;
			}
		}
		else
		{
			modes[0] = GameStyle_Arcade;
			mode_count++;
		}
	}


	static void ResetAet()
	{
		info_style_index = 0;
		aet::Stop(&sel_info_style[0]);
		aet::Stop(&sel_info_style[1]);
		aet::Stop(&sel_info_win);
		aet::Stop(&sel_info_txt);
	}

	static void SetAet()
	{
		if (mode_count < 1)
		{
			ResetAet();
			return;
		}

		if (sel_info_win == 0)
		{
			std::string win = GetWindowLayerName();
			sel_info_win = aet::PlayLayer(AetSelSceneID, 10, 0x10000, win.c_str(), nullptr, nullptr, nullptr);
		}

		if (sel_info_txt == 0)
		{
			std::string txt = GetWindowTextLayerName();
			sel_info_txt = aet::PlayLayer(AetSelSceneID, 10, 0x10000, txt.c_str(), nullptr, nullptr, nullptr);
		}
		
		if (modes[selected_index] != prev_sel_mode)
		{
			std::string layer = GetStyleLayerName(modes[selected_index]);

			if (sel_info_style[info_style_index] != 0)
			{
				std::string prev_layer = GetStyleLayerName(prev_sel_mode);
				aet::Stop(&sel_info_style[info_style_index]);
				sel_info_style[info_style_index] = aet::PlayLayer(AetSelSceneID, 10, prev_layer.c_str(), AetAction_OutOnce);

				if (++info_style_index > 1)
					info_style_index = 0;
			}

			aet::Stop(&sel_info_style[info_style_index]);
			sel_info_style[info_style_index] = aet::PlayLayer(AetSelSceneID, 10, layer.c_str(), AetAction_InLoop);
		}

		diva::vec3 offset;
		offset.x = game::IsFutureToneMode() && !IsModeChangeable() ? -18.0f : 0.0f;
		offset.y = 0.0f;
		offset.z = 0.0f;
		aet::SetPosition(sel_info_style[0], &offset);
		aet::SetPosition(sel_info_style[1], &offset);
	}

	static void SetAetVisibility(bool visible)
	{
		aet::SetVisible(sel_info_style[0], visible);
		aet::SetVisible(sel_info_style[1], visible);
		aet::SetVisible(sel_info_win, visible);
		aet::SetVisible(sel_info_txt, visible);
		ui_visible = visible;
	}

	static void DrawKeyIcon()
	{
		if (sel_info_win == 0 || !ui_visible || !IsModeChangeable())
			return;

		const uint32_t key_sprite_ids_nsw[6] = {
			3203571017, // Xbox
			2779278485, // DualShock 4
			2152960672, // JoyCon
			312905805,  // Steam Deck
			257587290,  // Keyboard
			0
		};

		const uint32_t key_sprite_ids_ps4[6] = {
			585831999,  // Xbox
			1474623112, // DualShock 4
			3007666668, // JoyCon
			1166812452, // Steam Deck
			2835797874, // Keyboard
			0
		};

		int32_t device = diva::GetInputState(0)->GetDevice();

		AetComposition comp;
		aet::GetComposition(&comp, pvsel::sel_info_win);

		if (auto it = comp.find("p_key_icon_c"); it != comp.end())
		{
			SprArgs args = { };
			args.id = game::IsFutureToneMode() ? key_sprite_ids_ps4[device] : key_sprite_ids_nsw[device];
			args.trans = it->second.position;
			args.scale.x = it->second.matrix.row0.x;
			args.scale.y = it->second.matrix.row1.y;
			args.scale.z = 1.0f;
			args.resolution_mode_sprite = 14;
			args.resolution_mode_screen = 14;
			memset(args.color, 0xFF, 4);
			args.attr = 0x400000; // SPR_ATTR_CTR_CC
			args.priority = 11;
			spr::DrawSprite(&args);
		}
	}

	static void SetStateSelectedMode()
	{
		if (modes[selected_index] == GameStyle_Arcade)
		{
			state.nc_song_entry.reset();
			state.nc_chart_entry.reset();
		}
		else
		{
			if (const db::SongEntry* song = db::FindSongEntry(cur_pv_id); song != nullptr)
			{
				state.nc_song_entry = *song;
				state.nc_chart_entry = db::FindDifficultyEntry(cur_pv_id, cur_diff, cur_edition)->charts[selected_index];
			}
		}
	}

	static void SetSelectedSong(int32_t pv, int32_t diff, int32_t ed)
	{
		if (pv != cur_pv_id || diff != cur_diff || ed != cur_edition)
		{
			prev_sel_mode = modes[selected_index];
			selected_index = 0;
			PopulateModes(pv, diff, ed);

			for (int i = 0; i < mode_count; i++)
			{
				if (modes[i] == prev_sel_mode)
					selected_index = i;
			}

			SetAet();
			SetStateSelectedMode();
		}

		cur_pv_id = pv;
		cur_diff = diff;
		cur_edition = ed;
	}

	static void ResetSelectedSong()
	{
		cur_pv_id = -1;
		cur_diff = -1;
		cur_edition = -1;
		prev_sel_mode = GameStyle_Max;
		selected_index = 0;
		mode_count = 0;
		modes[0] = GameStyle_Max;
		modes[1] = GameStyle_Max;
		modes[2] = GameStyle_Max;
	}

	static int32_t IterModeSelection()
	{
		if (mode_count == 0)
		{
			selected_index = -1;
			return GameStyle_Max;
		}

		prev_sel_mode = modes[selected_index];
		if (++selected_index >= mode_count)
			selected_index = 0;

		SetAet();
		SetStateSelectedMode();
		return modes[selected_index];
	}

	static void UpdateCtrl(int32_t pv_id, int32_t difficulty, int32_t edition)
	{
		if (!game::IsCustomizeSelTaskReady())
		{
			pvsel::SetAetVisibility(true);
			pvsel::SetSelectedSong(pv_id, difficulty, edition);

			diva::InputState* is = diva::GetInputState(0);
			if (is->IsButtonTapped(92) || is->IsButtonTapped(13)) // F7 or L2
			{
				pvsel::IterModeSelection();
				if (pvsel::IsModeChangeable())
					sound::PlaySoundEffect(1, "se_ft_music_selector_set_change_01", 1.0f);
			}
		}
		else
			pvsel::SetAetVisibility(false);
	}
	*/

	void RequestAssetsLoad()
	{
		prj::string str;
		prj::string_view strv;
		aet::LoadAetSet(AetSelSetID, &str);
		spr::LoadSprSet(SprSelSetID, &strv);
	}

	bool CheckAssetsLoaded()
	{
		assets_loaded = !aet::CheckAetSetLoading(AetSelSetID) && !spr::CheckSprSetLoading(SprSelSetID);
		return assets_loaded;
	}

	void UnloadAssets()
	{
		// ResetAet();
		aet::UnloadAetSet(AetSelSetID);
		spr::UnloadSprSet(SprSelSetID);
	}

	std::vector<uint8_t> FindAvailableStyles(const PvData* const* pvs, size_t count, int32_t difficulty, int32_t edition)
	{
		std::vector<uint8_t> data;
		data.reserve(GameStyle_Max);

		for (size_t i = 0; i < count; i++)
		{
			if (pvs[i] == nullptr || pvs[i]->data2 == nullptr)
				continue;

			const PvData& pv = *pvs[i];
			int32_t pv_id = *pv.data2->pv_db_entry;
			if (auto* entry = db::FindDifficultyEntry(pv_id, difficulty, edition); entry != nullptr)
			{
				for (auto& chart : entry->charts)
				{
					for (int32_t i = 0; i < GameStyle_Max; i++)
					{
						if (chart.style == i && std::find(data.begin(), data.end(), i) == data.end())
							data.push_back(i);
					}
				}
			}
		}

		if (data.empty())
			data.push_back(GameStyle_Arcade);

		return data;
	}

	bool IsSongAvailableInCurrentStyle(const PvData* pv_data, int32_t difficulty, int32_t edition)
	{
		if (!pv_data || !pv_data->data2 || !pv_data->data2->pv_db_entry)
			return false;

		auto* entry = db::FindDifficultyEntry(*pv_data->data2->pv_db_entry, difficulty, edition);
		if (entry != nullptr)
		{
			for (auto& chart : entry->charts)
				if (chart.style == GetSelectedStyleOrDefault())
					return true;
		}

		return false;
	}
}

struct SelPvData
{
	uint8_t data[0x48];
};

struct PvData2
{
	int32_t* pv_db_entry;
	uint8_t gap08[120];
	int32_t name_sort_index;
};

static bool force_console_style_only = true;

HOOK(bool, __fastcall, CheckSongElligibleInCategory, 0x140582140,
	PvData2* pv, int32_t a2, int32_t a3, int32_t sort_mode, int32_t difficulty, int32_t edition)
{
	/*
	nc::Print("INFO: pv=%d sort_index=%d\n",
		*pv->pv_db_entry,
		pv->name_sort_index
	);
	*/

	bool ret = originalCheckSongElligibleInCategory(pv, a2, a3, sort_mode, difficulty, edition);

	nc::Print("INFO: pv=%d diff=%d(%d) sort_index=%d info=%d/%d sort_mode=%d include=%d\n",
		*pv->pv_db_entry,
		difficulty,
		edition,
		pv->name_sort_index,
		a2,
		a3,
		sort_mode,
		ret
	);

	/*
	nc::Print("INFO: %d <-> %d\n",
		**reinterpret_cast<int32_t**>(a1 + 0x26240),
		*reinterpret_cast<int32_t*>(a1 + 0x2629C)
	);
	*/

	if (force_console_style_only)
	{
		const db::DifficultyEntry* nc_entry = db::FindDifficultyEntry(*pv->pv_db_entry, difficulty, edition);
		if (nc_entry != nullptr)
		{
			for (auto& chart : nc_entry->charts)
				if (chart.style == GameStyle_Console)
					return ret;
		}

		return false;
	}

	return ret;
}

void InstallPvSelSwitchHooks(); // NOTE: Defined in pv_sel_switch.cpp
void InstallPvSelPS4Hooks();    // NOTE: Defined in pv_sel_ps4.cpp

void InstallPvSelHooks()
{
	if (game::IsFutureToneMode())
		InstallPvSelPS4Hooks();
	else
		InstallPvSelSwitchHooks();
}