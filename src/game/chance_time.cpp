#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <common.h>
#include "chance_time.h"

static FUNCTION_PTR(void, __fastcall, SetNormalFrameAction, 0x1402756F0, PVGameUI* a1, bool in, float length_sec);
static FUNCTION_PTR(void, __fastcall, SetFrameAction, 0x140278AB0, PVGameUI* a1, bool visible, int32_t index, int32_t action, float length);

enum LayerUI : int32_t
{
	LayerUI_ChanceFrameTop = 0,
	LayerUI_ChanceFrameBottom,
	LayerUI_StarGaugeBase,
	LayerUI_StarGauge,
	LayerUI_Max
};

struct UIState
{
	int32_t aet_list[LayerUI_Max];
	bool visibility[LayerUI_Max];

	UIState()
	{
		memset(aet_list, 0, sizeof(aet_list));
		memset(visibility, 0, sizeof(visibility));
	}

	void SetLayer(int32_t index, bool visible, const char* name, int32_t prio, int32_t flags)
	{
		diva::aet::Stop(&aet_list[index]);
		aet_list[index] = diva::aet::PlayLayer(AetSceneID, prio, flags, name, nullptr, 0, nullptr, nullptr, -1.0f, 1.0f, 0, nullptr);
		visibility[index] = visible;
	}
} static ui_state;

HOOK(void, __fastcall, ExecuteModeSelect, 0x1503B04A0, PVGamePvData* pv_data, int32_t op)
{
	int32_t op_difficulty = pv_data->script_buffer[pv_data->script_pos + 1];
	int32_t difficulty = 1 << diva::GetPvGameplayInfo()->difficulty;
	if ((op_difficulty & difficulty) != 0)
	{
		switch (pv_data->script_buffer[pv_data->script_pos + 2])
		{
		case 4:
			// Chance Time Start
			SetNormalFrameAction(&pv_data->pv_game->ui, false, 1.0f);
			SetFrameAction(&pv_data->pv_game->ui, false, 1, 3, 60.0f);
			pv_data->pv_game->ui.draw_combo_counter = true;

			ui_state.SetLayer(LayerUI_ChanceFrameTop, true, "chance_frame_top", 12, 0x10000);
			ui_state.SetLayer(LayerUI_ChanceFrameBottom, true, "chance_frame_bottom", 12, 0x10000);
			ui_state.SetLayer(LayerUI_StarGauge, true, "gauge_ch00", 13, 0x10000);
			break;
		case 5:
			// Chance Time End
			SetNormalFrameAction(&pv_data->pv_game->ui, true, 1.0f);
			SetFrameAction(&pv_data->pv_game->ui, false, 1, 4, 60.0f);
			break;
		}
	}

	return originalExecuteModeSelect(pv_data, op);
}

HOOK(void, __fastcall, UpdateGaugeFrame, 0x14027A490, PVGameUI* ui)
{
	originalUpdateGaugeFrame(ui);

	if (ui_state.aet_list[LayerUI_StarGauge] != 0)
	{
		char name[32];
		sprintf_s(name, "gauge_ch%02d", state.chance_time.GetFillRate());

		ui_state.SetLayer(LayerUI_StarGauge, true, name, 13, 0x10000);
	}

	// NOTE: Set UI position
	diva::vec3 top = { 0.0f, -ui->frame_top_offset[1], 0.0f };
	diva::vec3 bottom = { 0.0f, ui->frame_bottom_offset[1], 0.0f };

	diva::aet::SetPosition(ui_state.aet_list[LayerUI_ChanceFrameTop], &top);
	diva::aet::SetPosition(ui_state.aet_list[LayerUI_ChanceFrameBottom], &bottom);
	diva::aet::SetPosition(ui_state.aet_list[LayerUI_StarGauge], &bottom);
}

void FinishChanceTimeUI()
{
	for (int i = 0; i < LayerUI_Max; i++)
	{
		diva::aet::Stop(&ui_state.aet_list[i]);
		ui_state.visibility[i] = false;
	}
}

void InstallChanceTimeHooks()
{
	INSTALL_HOOK(ExecuteModeSelect);
	INSTALL_HOOK(UpdateGaugeFrame);
}