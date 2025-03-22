#include <stdint.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <nc_log.h>
#include <diva.h>
#include <nc_state.h>

enum GameStyle : int32_t
{
	GameStyle_Arcade  = 0,
	GameStyle_Console = 1,
	GameStyle_Mixed   = 2
};

int prev_state = -1;
int32_t cur_pv_id = -1;
int32_t cur_difficulty = -1;
int32_t cur_edition = -1;
int32_t game_style_count = 0;
int32_t cur_sel_style = 0;
int32_t game_styles[3] = { 0, 0, 0 };

static bool CheckPVHasNC(int32_t id, int32_t diff_index, int32_t edition)
{
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

HOOK(bool, __fastcall, PVSelectorSwitchCtrl, 0x1406EDC40, uint64_t a1)
{
	int32_t* sel_state = reinterpret_cast<int32_t*>(a1 + 108);
	int32_t* pv_id = reinterpret_cast<int32_t*>(a1 + 0x262A4);
	int32_t* difficulty = reinterpret_cast<int32_t*>(a1 + 0x262C8);
	int32_t* edition = reinterpret_cast<int32_t*>(a1 + 0x262CC);

	bool ret = originalPVSelectorSwitchCtrl(a1);

	if (*sel_state == 6)
	{
		if (*pv_id != cur_pv_id || *difficulty != cur_difficulty || *edition != cur_edition)
		{
			// nc::Print("PV Selector: %d %d %d %d\n", *pv_id, *difficulty, *edition, CheckPVHasNC(*pv_id, *difficulty, *edition));

			game_style_count = 0;
			cur_sel_style = 0;
			state.song_mode = SongMode_Original;
			game_styles[game_style_count++] = GameStyle_Arcade;

			if (CheckPVHasNC(*pv_id, *difficulty, *edition))
				game_styles[game_style_count++] = GameStyle_Console;

			cur_pv_id = *pv_id;
			cur_difficulty = *difficulty;
			cur_edition = *edition;
		}

		if (diva::GetInputState(0)->IsButtonTapped(92)) // KEYBOARD_4
		{
			cur_sel_style++;
			if (cur_sel_style >= game_style_count)
				cur_sel_style = 0;

			switch (cur_sel_style)
			{
			case GameStyle_Arcade:
				nc::Print("Selected Game Style: Arcade\n");
				state.song_mode = SongMode_Original;
				break;
			case GameStyle_Console:
				nc::Print("Selected Game Style: Console\n");
				state.song_mode = SongMode_NC;
				break;
			case GameStyle_Mixed:
				nc::Print("Selected Game Style: Mixed\n");
				state.song_mode = SongMode_NC;
				break;
			}			
		}
	}

	return ret;
}

void InstallPvSelHooks()
{
	INSTALL_HOOK(PVSelectorSwitchCtrl);
}