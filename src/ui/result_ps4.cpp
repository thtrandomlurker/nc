#include <stdint.h>
#include <hooks.h>
#include <util.h>
#include <nc_state.h>
#include "common.h"
#include "result.h"

struct StageResultPS4
{
	uint8_t gap0[104];
	int32_t state;
	int32_t cmn_bg;
	int32_t win_base;
	int32_t win;
	int32_t win_almost;
	int32_t win_achieve;
	int32_t win_score;
	int32_t logo_img;
	int32_t jk_img;
	int32_t tit_clear;
	int32_t tit_value;
	int32_t eff_clear;
	int32_t eff_record;
	int32_t eff_clear_ahv;
	int32_t win_option_base;
	int32_t win_option;
	int32_t dwordA8;
	int32_t result_c;
	uint8_t gapB0[8];
	int32_t result_kansou_tit;
	int32_t belt_gam_rslt;
	int32_t rank_max_f;
	int32_t rank_max_t;
	int32_t result_tit;
	int32_t g_level;
	uint8_t gapD0[4];
	int32_t result_survival_tit;
	uint8_t gapD8[48];
	ScoreDetail* detail;
	uint8_t gap110[8];
	char char118;
	uint8_t gap119[23];
	uint64_t qword130;
	std::string win_base_name;
	std::string win_name;
	uint8_t gap178[199];
	bool score_win_in;
	uint8_t gap240[56];
	uint8_t byte278;
	uint8_t byte279;
};

class StyleBelt : AetElement
{
private:
	AetElement style_txt;

public:
	StyleBelt()
	{
		SetScene(14010071);
		SetLayer("ps4_gam_belt_rslt", 3, 14, AetAction_InOnce);
	}

	void Ctrl() override
	{
		if (Ended())
		{
			SetLayer("ps4_gam_belt_rslt", 3, 14, AetAction_Loop);
			style_txt.SetScene(14010071);
			style_txt.SetLayer("ps4_tit_console_en", 3, 14, AetAction_InLoop);
		}

		if (auto layout = GetLayout("p_style_loc_c"); layout.has_value())
		{
			diva::vec3 center = diva::vec3(layout.value().width / 2.0f, layout.value().height / 2.0f, 0.0f);
			diva::vec3 offset = layout.value().anchor - center;
			style_txt.SetPosition(layout.value().position - offset);
		}
	}
};

static std::unique_ptr<StyleBelt> belt;

static inline void WriteDispAetSceneID(uint32_t id) { WRITE_MEMORY(0x1402334AE, uint32_t, id); }

static void SetWindowAet(StageResultPS4* result, const std::string& name, bool loop = false)
{
	if (!nc::ShouldUseConsoleStyleWin())
		return;

	aet::Stop(&result->win);
	result->win = aet::PlayLayer(14010071, 3, loop ? 0x10000 : 0x20000, name.c_str(), nullptr, nullptr, nullptr);
	result->win_name = name;
}

HOOK(void, __fastcall, PutScoreWindowIn, 0x1402365C0, StageResultPS4* result)
{
	originalPutScoreWindowIn(result);
	SetWindowAet(result, "ps4_win_nc_in_en");
}

HOOK(void, __fastcall, PutWinCount, 0x1402367C0, StageResultPS4* result)
{
	originalPutWinCount(result);
	SetWindowAet(result, "ps4_win_nc_count_en");
}

HOOK(void, __fastcall, PutWinLoop, 0x140236A30, StageResultPS4* result)
{
	originalPutWinLoop(result);
	SetWindowAet(result, "ps4_win_nc_loop_en", true);
}

HOOK(void, __fastcall, PutWinOut, 0x140236C80, StageResultPS4* result)
{
	originalPutWinOut(result);
	SetWindowAet(result, "ps4_win_nc_out_en");
}

HOOK(bool, __fastcall, StageResultPS4Init, 0x140231A70, StageResultPS4* result)
{
	bool ret = originalStageResultPS4Init(result);

	prj::string str;
	prj::string_view strv;
	aet::LoadAetSet(14010070, &str);
	spr::LoadSprSet(14020070, &strv);

	nc::InitResultsData(result->detail);
	WriteDispAetSceneID(nc::ShouldUseConsoleStyleWin() ? 14010071 : 1279);

	return ret;
}

HOOK(bool, __fastcall, StageResultPS4Ctrl, 0x140232010, StageResultPS4* result)
{
	if (result->state == 0)
	{
		if (aet::CheckAetSetLoading(14010070) || spr::CheckSprSetLoading(14020070))
			return false;
	}

	bool ret = originalStageResultPS4Ctrl(result);

	if (result->state == 10)
	{
		if (!belt)
			belt = std::make_unique<StyleBelt>();
	}

	if (belt)
		belt->Ctrl();

	return ret;
}

HOOK(bool, __fastcall, StageResultPS4Dest, 0x140232DD0, uint64_t a1)
{
	belt.reset();
	aet::UnloadAetSet(14010070);
	spr::UnloadSprSet(14020070);
	return originalStageResultPS4Dest(a1);
}

HOOK(void, __fastcall, StageResultPS4Disp, 0x140232E80, StageResultPS4* result)
{
	originalStageResultPS4Disp(result);
	if (!result->win)
		return;

	if (nc::ShouldUseConsoleStyleWin())
		nc::DrawResultsWindowText(result->win);
}

void InstallResultPS4Hooks()
{
	INSTALL_HOOK(PutScoreWindowIn);
	INSTALL_HOOK(PutWinCount);
	INSTALL_HOOK(PutWinLoop);
	INSTALL_HOOK(PutWinOut);
	INSTALL_HOOK(StageResultPS4Init);
	INSTALL_HOOK(StageResultPS4Ctrl);
	INSTALL_HOOK(StageResultPS4Dest);
	INSTALL_HOOK(StageResultPS4Disp);
}