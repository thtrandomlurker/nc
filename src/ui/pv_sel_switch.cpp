#include <stdint.h>
#include <hooks.h>
#include <save_data.h>
#include "pv_sel.h"

struct PVSelectorSwitch
{
	INSERT_PADDING(0x6C);
	int32_t state;
	INSERT_PADDING(0x258C0);
	pvsel::SelPvList sel_pv_list;
	INSERT_PADDING(0x498);
	prj::vector<pvsel::PvData> sorted_pv_lists[32];
	prj::vector<pvsel::PvData>* cur_pv_list;
	INSERT_PADDING(0x8);
	pvsel::PvData random_pv;
	int32_t* cur_sort_index;
	int32_t song_counts[21];
	int32_t all_sort_index;
	int32_t dword262A0;
	int32_t pv_id;
	int32_t dword262A8;
	int32_t sort_indices[4];
	int32_t sort_change_dir;
	int32_t sort_mode;
	uint8_t difficulty_max;
	uint8_t difficulty_min;
	INSERT_PADDING(0x2);
	int32_t difficulty;
	int32_t edition;
};

struct StateKeepSwitch
{
	bool style_dirty = false;
	bool disable_refresh = false;
	int32_t pv_id = -1;
} static state_tmp;

static inline void RefreshAvailableStyles(const PVSelectorSwitch* sel)
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

static inline void SetGlobalStateSelectedData(const PVSelectorSwitch* sel)
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

static FUNCTION_PTR(void, __fastcall, PVSelectorSwitchChangeSortFilter, 0x1406F2F40, PVSelectorSwitch*, int32_t);
static FUNCTION_PTR(void, __fastcall, PVSelectorSwitchCalculateSongCountPerCategory, 0x1406F3E10, PVSelectorSwitch*);
static FUNCTION_PTR(void*, __fastcall, sub_1406DE410, 0x1406DE410, uint64_t);
static FUNCTION_PTR(void, __fastcall, UpdateBelt, 0x1406DC400, void*, int32_t difficulty, int32_t edition);
static FUNCTION_PTR(void, __fastcall, sub_1406F2670, 0x1406F2670, void*, bool, bool, bool);

HOOK(void, __fastcall, PVSelectorSwitchChangeDifficulty, 0x1406F3A20, PVSelectorSwitch* sel, int32_t dir)
{
	do
	{
		sel->difficulty += dir;
		if ((sel->difficulty == sel->difficulty_max || sel->difficulty < sel->difficulty_min) && sel->edition == 0)
		{
			sel->difficulty = sel->difficulty_max - 1;
			sel->edition = 1;
		}
		else if (sel->edition == 1)
		{
			sel->difficulty = (sel->difficulty == sel->difficulty_max) ? sel->difficulty_min : sel->difficulty_max - 1;
			sel->edition = 0;
		}

		sel->cur_pv_list = &sel->sorted_pv_lists[sel->edition + 2 * (sel->sort_mode * 4 + sel->difficulty)];
	} while (sel->cur_pv_list->size() == 0);

	// NOTE: Call change sort filter once before looking for available game styles,
	//       so the game properly changes the sort filter index if needed.
	//       This prevents crashes when, for example, sorting for 8☆ Extremes and
	//       changing the difficulty to Hard, which doesn't have the 8☆ filter.
	state_tmp.disable_refresh = true;
	PVSelectorSwitchChangeSortFilter(sel, 0);

	int32_t cur_style = pvsel::GetSelectedStyleOrDefault();
	do
	{
		if (pvsel::gs_win)
		{
			if (cur_style != pvsel::GetSelectedStyleOrDefault())
				pvsel::gs_win->ForceSetOption(cur_style);
		}

		PVSelectorSwitchCalculateSongCountPerCategory(sel);

		if (++cur_style >= GameStyle_Max)
			cur_style = GameStyle_Arcade;
	} while (sel->song_counts[*sel->cur_sort_index] == 0);

	// NOTE: Call change sort filter again, which now will properly pull the correct
	//       list of PVs for the selected game style.
	state_tmp.disable_refresh = false;
	PVSelectorSwitchChangeSortFilter(sel, 0);

	UpdateBelt(sub_1406DE410(reinterpret_cast<uint64_t>(sel) + 6736), sel->difficulty, sel->edition);
	sub_1406F2670(sel, false, false, true);
}

HOOK(void, __fastcall, PVSelectorSwitchChangeSortFilterImp, 0x1406F2F40, PVSelectorSwitch* sel, int32_t dir)
{
	originalPVSelectorSwitchChangeSortFilterImp(sel, dir);
	if (!state_tmp.disable_refresh)
		RefreshAvailableStyles(sel);
}

HOOK(bool, __fastcall, PVSelectorSwitchCheckSongPertainsInCategory, 0x1406F3D40, 
	PVSelectorSwitch* a1, pvsel::PvData* pv_data, int32_t a3, int32_t a4)
{
	bool org = originalPVSelectorSwitchCheckSongPertainsInCategory(a1, pv_data, a3, a4);
	if (!org || pvsel::GetSelectedStyleOrDefault() == GameStyle_Arcade)
		return org;

	return org && pvsel::IsSongAvailableInCurrentStyle(pv_data, a1->difficulty, a1->edition);
}

HOOK(bool, __fastcall, PVSelectorSwitchInit, 0x1406ED9D0, uint64_t a1)
{
	pvsel::RequestAssetsLoad();
	if (!pvsel::gs_win)
	{
		pvsel::gs_win = std::make_unique<pvsel::GSWindow>();
		pvsel::gs_win->is_sort_mode = true;
	}

	state.nc_song_entry.reset();
	state.nc_song_entry.reset();

	bool ret = originalPVSelectorSwitchInit(a1);
	pvsel::gs_win->TrySetSelectedOption(nc::GetSharedData().pv_sel_selected_style);

	return ret;
}

HOOK(bool, __fastcall, PVSelectorSwitchCtrl, 0x1406EDC40, PVSelectorSwitch* sel)
{
	if (sel->state == 0)
	{
		if (!pvsel::CheckAssetsLoaded())
			return false;
	}

	bool ret = originalPVSelectorSwitchCtrl(sel);

	if (sel->state == 6)
	{
		if (pvsel::gs_win->Ctrl())
			PVSelectorSwitchChangeSortFilter(sel, 0);
	}
	
	SetGlobalStateSelectedData(sel);
	nc::GetSharedData().pv_sel_selected_style = pvsel::GetSelectedStyleOrDefault();
	return ret;
}

HOOK(bool, __fastcall, PVSelectorSwitchDest, 0x1406EE720, uint64_t a1)
{
	pvsel::gs_win = nullptr;
	pvsel::UnloadAssets();
	return originalPVSelectorSwitchDest(a1);
}

HOOK(void, __fastcall, PVSelectorSwitchDisp, 0x1406EE7B0, uint64_t a1)
{
	originalPVSelectorSwitchDisp(a1);
	if (pvsel::gs_win)
		pvsel::gs_win->Disp();
}

void InstallPvSelSwitchHooks()
{
	INSTALL_HOOK(PVSelectorSwitchChangeDifficulty);
	INSTALL_HOOK(PVSelectorSwitchChangeSortFilterImp);
	INSTALL_HOOK(PVSelectorSwitchCheckSongPertainsInCategory);
	INSTALL_HOOK(PVSelectorSwitchInit);
	INSTALL_HOOK(PVSelectorSwitchCtrl);
	INSTALL_HOOK(PVSelectorSwitchDest);
	INSTALL_HOOK(PVSelectorSwitchDisp);
}