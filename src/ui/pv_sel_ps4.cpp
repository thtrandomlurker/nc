#include <stdint.h>
#include <hooks.h>
#include "pv_sel.h"

struct PVselPS4
{
	INSERT_PADDING(0x6C);
	int32_t state;
	INSERT_PADDING(0x36E78);
	pvsel::SelPvList sel_pv_list;
	uint8_t sorted_pv_list_ref;
	INSERT_PADDING(0x31F);
	pvsel::PvData random_pv;
	INSERT_PADDING(0x8);
	int32_t song_counts[21];
	int32_t* cur_sort_index;
	int32_t dword373D8;
	int32_t pv_id;
	int32_t sort_mode;
	INSERT_PADDING(0x14);
	int32_t difficulty;
	int32_t edition;
};

struct StatePS4Keep
{
	bool style_dirty = false;
	int32_t pv_id = -1;
} static state_tmp;

static inline void RefreshAvailableStyles(PVselPS4* sel)
{
	if (sel->sel_pv_list.pv_data)
	{
		auto styles = pvsel::FindAvailableStyles(*sel->sel_pv_list.pv_data, sel->difficulty, sel->edition);
		if (pvsel::gs_win)
		{
			pvsel::gs_win->SetOptions(styles.data(), styles.size(), true);
			pvsel::gs_win->MakeAetDirty();
			state_tmp.style_dirty = true;
		}
	}
}

static inline void SetGlobalStateSelectedData(const PVselPS4* sel)
{
	int32_t pv = sel->pv_id;
	if (pv == -2 && sel->random_pv.data2 != nullptr)
		pv = *sel->random_pv.data2->pv_db_entry;

	if (state_tmp.style_dirty || pv != state_tmp.pv_id)
	{
		if (auto* entry = db::FindSongEntry(pv); entry != nullptr)
			state.nc_song_entry = *entry;

		if (auto* entry = db::FindChart(pv, sel->difficulty, sel->edition, pvsel::GetSelectedStyleOrDefault()); entry != nullptr)
			state.nc_chart_entry = *entry;

		state_tmp.pv_id = pv;
		state_tmp.style_dirty = false;
	}
}

static FUNCTION_PTR(bool, __fastcall, IsSurvivalMode, 0x14023B6A0);
static FUNCTION_PTR(void, __fastcall, PVselPS4ChangeSortFilter, 0x1402078D0, PVselPS4* sel, int32_t dir);
static FUNCTION_PTR(void, __fastcall, PVselPS4ChangeSurvivalCourse, 0x140207830, PVselPS4* sel, int32_t dir);
static FUNCTION_PTR(void, __fastcall, PVselPS4CalculateSongCountPerCategory, 0x140207AB0, PVselPS4* sel);
static FUNCTION_PTR(void, __fastcall, sub_140206C30, 0x140206C30, PVselPS4* sel);
static FUNCTION_PTR(void, __fastcall, sub_1402066C0, 0x1402066C0, PVselPS4* sel, bool a2, bool a3, bool a4);

HOOK(void, __fastcall, PVselPS4ChangeDifficulty, 0x140207550, PVselPS4* sel, int32_t dir)
{
	if (IsSurvivalMode())
	{
		PVselPS4ChangeSurvivalCourse(sel, dir);
		return;
	}

	uint64_t* cur_data;
	do
	{
		sel->difficulty += dir;
		if ((sel->difficulty == 4 || sel->difficulty < 0) && sel->edition == 0)
		{
			sel->difficulty = 3;
			sel->edition = 1;
		}
		else if (sel->edition == 1)
		{
			sel->difficulty = (sel->difficulty == 4) ? 0 : 3;
			sel->edition = 0;
		}

		uint8_t* data = &sel->sorted_pv_list_ref;
		cur_data = reinterpret_cast<uint64_t*>(&data[48 * sel->sort_mode * 4 + 48 * sel->difficulty + 24 * sel->edition]);
		*reinterpret_cast<void**>(&data[768]) = cur_data;
	} while (!((cur_data[1] - cur_data[0]) / 80));
	
	int32_t cur_style = pvsel::GetSelectedStyleOrDefault();
	do
	{
		if (pvsel::gs_win)
		{
			if (cur_style != pvsel::GetSelectedStyleOrDefault())
				pvsel::gs_win->ForceSetOption(cur_style);
		}

		PVselPS4CalculateSongCountPerCategory(sel);

		if (++cur_style >= GameStyle_Max)
			cur_style = GameStyle_Arcade;
	} while (sel->song_counts[*sel->cur_sort_index] == 0);

	RefreshAvailableStyles(sel);
	sub_140206C30(sel);
	if (dir)
		sub_1402066C0(sel, false, false, true);
}

HOOK(void, __fastcall, PVselPS4ChangeSortFilterImp, 0x1402078D0, PVselPS4* sel, int32_t dir)
{
	originalPVselPS4ChangeSortFilterImp(sel, dir);
	if (!IsSurvivalMode())
		RefreshAvailableStyles(sel);
}

HOOK(bool, __fastcall, PVselPS4CheckSongPertainsInCategory, 0x1402079E0,
	PVselPS4* sel, pvsel::PvData* pv_data, int32_t a3, int32_t a4)
{
	bool org = originalPVselPS4CheckSongPertainsInCategory(sel, pv_data, a3, a4);
	if (!org || pvsel::GetSelectedStyleOrDefault() == GameStyle_Arcade)
		return org;

	return org && pvsel::IsSongAvailableInCurrentStyle(pv_data, sel->difficulty, sel->edition);
}

HOOK(bool, __fastcall, PVselPS4Init, 0x140202D50, PVselPS4* sel)
{
	bool ret = originalPVselPS4Init(sel);

	pvsel::RequestAssetsLoad();
	if (!pvsel::gs_win)
	{
		pvsel::gs_win = std::make_unique<pvsel::GSWindow>();
		pvsel::gs_win->is_sort_mode = true;
	}

	state.nc_song_entry.reset();
	state.nc_chart_entry.reset();
	RefreshAvailableStyles(sel);

	return ret;
}

HOOK(bool, __fastcall, PVselPS4Ctrl, 0x1402033C0, PVselPS4* sel)
{
	if (sel->state == 0)
	{
		if (!pvsel::CheckAssetsLoaded())
			return false;
	}

	bool ret = originalPVselPS4Ctrl(sel);

	if (sel->state == 6)
	{
		if (pvsel::gs_win->Ctrl())
			PVselPS4ChangeSortFilter(sel, 0);
	}

	SetGlobalStateSelectedData(sel);
	return ret;
}

HOOK(bool, __fastcall, PVselPS4Dest, 0x140204DA0, uint64_t a1)
{
	pvsel::gs_win = nullptr;
	pvsel::UnloadAssets();
	return originalPVselPS4Dest(a1);
}

HOOK(void, __fastcall, PVselPS4Disp, 0x140204EB0, uint64_t a1)
{
	originalPVselPS4Disp(a1);
	if (pvsel::gs_win)
		pvsel::gs_win->Disp();
}

void InstallPvSelPS4Hooks()
{
	INSTALL_HOOK(PVselPS4ChangeDifficulty);
	INSTALL_HOOK(PVselPS4ChangeSortFilterImp);
	INSTALL_HOOK(PVselPS4CheckSongPertainsInCategory);
	INSTALL_HOOK(PVselPS4Init);
	INSTALL_HOOK(PVselPS4Ctrl);
	INSTALL_HOOK(PVselPS4Dest);
	INSTALL_HOOK(PVselPS4Disp);
}