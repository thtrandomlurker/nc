#include <stdint.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <nc_log.h>
#include <diva.h>
#include <nc_state.h>

constexpr uint32_t AetSelSetID   = 14010050;
constexpr uint32_t AetSelSceneID = 14010051;
constexpr uint32_t SprSelSetID   = 14020050;

enum GameStyle : int32_t
{
	GameStyle_Arcade  = 0,
	GameStyle_Console = 1,
	GameStyle_Mixed   = 2,
	GameStyle_Max
};

static std::string GetWindowLayerName(bool ps4)
{
	return ps4 ? "ps4_game_style_win" : "nsw_game_style_win";
}

static std::string GetStyleLayerName(int32_t style, bool ps4)
{
	if (style >= GameStyle_Max)
		return "";

	const char* nsw[3] = { "nsw_game_style_arcade", "nsw_game_style_console", "nsw_game_style_mixed" };
	const char* ft[3] = { "ps4_game_style_arcade", "ps4_game_style_console", "ps4_game_style_mixed" };
	return ps4 ? ft[style] : nsw[style];
}

static bool CheckPVHasNC(int32_t id, int32_t diff_index, int32_t edition)
{
	// TODO: Change this to use the pv_db script path instead
	//       (as that's how the script is loaded, this method creates some inconsistencies)
	if (id > 0 && diff_index >= 0 && diff_index < 5 && edition >= 0 && edition < 2)
	{
		const char* difficulties[5] = { "easy", "normal", "hard", "extreme", "encore" };
		char filename[256] = { 0 };

		if (edition > 0)
			sprintf_s(filename, "rom/script/pv_%03d_%s_%d_nc.dsc", id, difficulties[diff_index], edition);
		else
			sprintf_s(filename, "rom/script/pv_%03d_%s_nc.dsc", id, difficulties[diff_index]);

		prj::string path = filename;
		prj::string fixed;
		return FileCheckExists(&path, &fixed);
	}

	return false;
}

namespace pvsel
{
	bool assets_loaded     = false;
	int32_t sel_info_style = 0;
	int32_t sel_info_win   = 0;

	int32_t cur_pv_id     = -1;
	int32_t cur_diff      = -1;
	int32_t cur_edition   = -1;
	int32_t selected_mode = 0;
	int32_t mode_count    = 0;
	int32_t modes[3]      = { GameStyle_Max, GameStyle_Max, GameStyle_Max };

	static void PopulateModes(int32_t pv, int32_t diff, int32_t ed)
	{
		// TODO: Change this so that songs don't necessarily need an arcade mode
		//
		modes[0] = GameStyle_Arcade;
		mode_count = 1;
		selected_mode = 0;

		if (CheckPVHasNC(pv, diff, ed))
			modes[mode_count++] = GameStyle_Console;
	}

	static void SetAet()
	{
		if (mode_count < 1)
		{
			aet::Stop(&sel_info_win);
			aet::Stop(&sel_info_style);
			return;
		}

		// TODO: Add support for PS4 UI
		if (sel_info_win == 0)
		{
			std::string win = GetWindowLayerName(false);
			sel_info_win = aet::PlayLayer(AetSelSceneID, 10, 0x10000, win.c_str(), nullptr, nullptr, nullptr);
		}

		// NOTE: This will change when BricOOtaku updates the aet
		aet::Stop(&sel_info_style);
		std::string st = GetStyleLayerName(modes[selected_mode], false);
		sel_info_style = aet::PlayLayer(AetSelSceneID, 10, 0x10000, st.c_str(), nullptr, nullptr, nullptr);
	}

	static void SetAetVisibility(bool visible)
	{
		aet::SetVisible(sel_info_style, visible);
		aet::SetVisible(sel_info_win, visible);
	}

	static void SetStateSelectedMode()
	{
		if (modes[selected_mode] == GameStyle_Console || modes[selected_mode] == GameStyle_Mixed)
			state.song_mode = SongMode_NC;
		else
			state.song_mode = SongMode_Original;
	}

	static void SetSelectedSong(int32_t pv, int32_t diff, int32_t ed)
	{
		if (pv != cur_pv_id || diff != cur_diff || ed != cur_edition)
		{
			int32_t previous_mode = modes[selected_mode];
			PopulateModes(pv, diff, ed);

			for (int i = 0; i < mode_count; i++)
			{
				if (modes[i] == previous_mode)
					selected_mode = i;
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
		selected_mode = 0;
		mode_count = 0;
	}

	static int32_t IterModeSelection()
	{
		if (mode_count == 0)
		{
			selected_mode = -1;
			return GameStyle_Max;
		}

		if (++selected_mode >= mode_count)
			selected_mode = 0;

		SetAet();
		SetStateSelectedMode();
		return modes[selected_mode];
	}

	static bool IsModeChangeable() { return mode_count > 1; }

	static void RequestAetLoad()
	{
		prj::string str;
		prj::string_view strv;
		aet::LoadAetSet(AetSelSetID, &str);
		spr::LoadSprSet(SprSelSetID, &strv);
	}

	static bool CheckAssetsLoading()
	{
		assets_loaded = !aet::CheckAetSetLoading(AetSelSetID) && !spr::CheckSprSetLoading(SprSelSetID);
		return assets_loaded;
	}

	static void UnloadAet()
	{
		aet::Stop(&sel_info_style);
		aet::Stop(&sel_info_win);
		aet::UnloadAetSet(AetSelSetID);
		spr::UnloadSprSet(SprSelSetID);
	}
}

static FUNCTION_PTR(bool, __fastcall, CheckCustomizeSelReady, 0x1401DE790);

HOOK(bool, __fastcall, PVSelectorSwitchInit, 0x1406ED9D0, uint64_t a1)
{
	pvsel::RequestAetLoad();
	return originalPVSelectorSwitchInit(a1);
}

HOOK(bool, __fastcall, PVSelectorSwitchCtrl, 0x1406EDC40, uint64_t a1)
{
	int32_t* sel_state = reinterpret_cast<int32_t*>(a1 + 108);
	int32_t* pv_id = reinterpret_cast<int32_t*>(a1 + 0x262A4);
	int32_t* difficulty = reinterpret_cast<int32_t*>(a1 + 0x262C8);
	int32_t* edition = reinterpret_cast<int32_t*>(a1 + 0x262CC);

	if (*sel_state == 0)
	{
		if (!pvsel::CheckAssetsLoading())
			return false;
	}

	bool ret = originalPVSelectorSwitchCtrl(a1);

	if (*sel_state == 6)
	{
		if (!CheckCustomizeSelReady())
		{
			pvsel::SetAetVisibility(true);
			pvsel::SetSelectedSong(*pv_id, *difficulty, *edition);

			diva::InputState* is = diva::GetInputState(0);
			if (is->IsButtonTapped(92))
			{
				pvsel::IterModeSelection();
				if (pvsel::IsModeChangeable())
					sound::PlaySoundEffect(1, "se_ft_music_selector_set_change_01", 1.0f);
			}
		}
		else
			pvsel::SetAetVisibility(false);
	}

	return ret;
}

HOOK(bool, __fastcall, PVSelectorSwitchDest, 0x1406EE720, uint64_t a1)
{
	pvsel::ResetSelectedSong();
	pvsel::UnloadAet();
	return originalPVSelectorSwitchDest(a1);
}

void InstallPvSelHooks()
{
	INSTALL_HOOK(PVSelectorSwitchInit);
	INSTALL_HOOK(PVSelectorSwitchCtrl);
	INSTALL_HOOK(PVSelectorSwitchDest);
}