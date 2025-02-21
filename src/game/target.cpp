#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <common.h>
#include "note_common.h"
#include "note_long.h"
#include "target.h"

static const NoteSprite nc_target_layers[] = {
	{ nullptr,                nullptr, nullptr }, // 0x19 - Triangle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1A - Circle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1B - Cross Rush
	{ nullptr,                nullptr, nullptr }, // 0x1C - Square Rush
	{ "target_up_w",          nullptr, nullptr }, // 0x1D - Triangle W
	{ "target_right_w",       nullptr, nullptr }, // 0x1E - Circle W
	{ "target_down_w",        nullptr, nullptr }, // 0x1F - Cross W
	{ "target_left_w",        nullptr, nullptr }, // 0x20 - Square W
	{ "target_sankaku_long",  nullptr, nullptr }, // 0x21 - Triangle Long
	{ "target_maru_long",     nullptr, nullptr }, // 0x22 - Circle Long
	{ "target_batsu_long",    nullptr, nullptr }, // 0x23 - Cross Long
	{ "target_shikaku_long",  nullptr, nullptr }, // 0x24 - Square Long
	{ "target_touch",         nullptr, nullptr }, // 0x25 - Star
	{ nullptr,                nullptr, nullptr }, // 0x26 - Star Long
	{ "target_touch_w",       nullptr, nullptr }, // 0x27 - Star W
	{ "target_touch_ch_miss", nullptr, nullptr }, // 0x28 - Chance Star
	{ "target_link",          nullptr, nullptr }, // 0x29 - Link Star
	{ "target_link",          nullptr, nullptr }, // 0x2A - Link Star End
};

static const NoteSprite nc_button_layers[] = {
	{ nullptr,                nullptr, nullptr }, // 0x19 - Triangle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1A - Circle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1B - Cross Rush
	{ nullptr,                nullptr, nullptr }, // 0x1C - Square Rush
	{ "button_up_w",          nullptr, nullptr }, // 0x1D - Triangle W
	{ "button_right_w",       nullptr, nullptr }, // 0x1E - Circle W
	{ "button_down_w",        nullptr, nullptr }, // 0x1F - Cross W
	{ "button_left_w",        nullptr, nullptr }, // 0x20 - Square W
	{ "button_sankaku_long",  nullptr, nullptr }, // 0x21 - Triangle Long
	{ "button_maru_long",     nullptr, nullptr }, // 0x22 - Circle Long
	{ "button_batsu_long",    nullptr, nullptr }, // 0x23 - Cross Long
	{ "button_shikaku_long",  nullptr, nullptr }, // 0x24 - Square Long
	{ "button_touch",         nullptr, nullptr }, // 0x25 - Star
	{ nullptr,                nullptr, nullptr }, // 0x26 - Star Long
	{ "button_touch_w",       nullptr, nullptr }, // 0x27 - Star W
	{ "button_touch_ch_miss", nullptr, nullptr }, // 0x28 - Chance Star
	{ "button_link",          nullptr, nullptr }, // 0x29 - Link Star
	{ "button_link",          nullptr, nullptr }, // 0x2A - Link Star End
};

static const char* GetProperLayerName(const NoteSprite* spr)
{
	// TODO: Add style logic
	if (spr->ft != nullptr)
		return spr->ft;

	return "target_sankaku";
}

const char* GetTargetLayer(int32_t target_type)
{
	if (target_type >= TargetType_Custom && target_type < TargetType_Max)
		return GetProperLayerName(&nc_target_layers[target_type - TargetType_Custom]);
	return nullptr;
}

const char* GetButtonLayer(int32_t target_type)
{
	if (target_type >= TargetType_Custom && target_type < TargetType_Max)
		return GetProperLayerName(&nc_button_layers[target_type - TargetType_Custom]);
	return nullptr;
}

int32_t PlayUpdateSoundEffect(PvGameTarget* target, TargetStateEx* ex, SoundEffect* se)
{
	switch (ex->target_type)
	{
	case TargetType_TriangleLong:
	case TargetType_CircleLong:
	case TargetType_CrossLong:
	case TargetType_SquareLong:
		if (ex != nullptr)
		{
			if (ex->se_state == SEState_Idle)
			{
				diva::sound::PlaySoundEffect(3, "800_button_l1_on", 1.0f);
				ex->se_queue = 3;
				ex->se_name = "800_button_l1_on";
				ex->se_state = SEState_Looping;
			}
			else if (ex->se_state == SEState_FailRelease)
			{
				diva::sound::ReleaseCue(ex->se_queue, ex->se_name.c_str(), false);
				ex->ResetSE();
			}
			else if (ex->se_state == SEState_SuccessRelease)
			{
				int32_t queue = ex->se_queue;
				diva::sound::ReleaseCue(ex->se_queue, ex->se_name.c_str(), true);
				ex->ResetSE();
				diva::sound::PlaySoundEffect(queue, "800_button_l1_off", 1.0f);
			}
		}

		return 1;
	case TargetType_UpW:
	case TargetType_RightW:
	case TargetType_DownW:
	case TargetType_LeftW:
		diva::sound::PlaySoundEffect(3, "400_button_w1", 1.0f);
		return 1;
	case TargetType_Star:
	case TargetType_LinkStar:
	case TargetType_LinkStarEnd:
	case TargetType_StarRush:
		diva::sound::PlaySoundEffect(3, "1200_scratch1", 1.0f);
		return 1;
	case TargetType_ChanceStar:
		if (ex != nullptr && ex->success)
		{
			diva::sound::PlaySoundEffect(3, "1516_cymbal", 1.0f);
			return 1;
		}

		diva::sound::PlaySoundEffect(3, "1200_scratch1", 1.0f);
		return 1;
	case TargetType_StarW:
		diva::sound::PlaySoundEffect(3, "1400_scratch_w1", 1.0f);
		return 1;
	}

	return 0;
};

/*
HOOK(void, __fastcall, UpdateTargets, 0x14026DD80, PVGameArcade* data, float dt)
{
	for (PvGameTarget* target = data->target; target != nullptr; target = target->next)
	{
		//
	}

	return originalUpdateTargets(data, dt);
}
*/

HOOK(void, __fastcall, PVGameReset, 0x1402436F0, void* pv_game)
{
	printf("[NC] Reset!\n");
	
	state.ResetPlayState();
	originalPVGameReset(pv_game);
}

HOOK(void, __fastcall, UpdateKiseki, 0x14026F050, PVGameArcade* data, PvGameTarget* target, float dt)
{
	if (target->target_type < TargetType_Custom)
		return originalUpdateKiseki(data, target, dt);

	if (IsLongNote(target->target_type))
	{
		if (TargetStateEx* ex = GetTargetStateEx(target->target_index); ex != nullptr)
		{
			ex->kiseki_pos = target->button_pos;
			ex->kiseki_dir = target->delta_pos_sq;
			UpdateLongNoteKiseki(data, target, ex, dt);
		}
	}
	else
	{
		originalUpdateKiseki(data, target, dt);
		PatchCommonKisekiColor(target);
	}
}

HOOK(void, __fastcall, DrawKiseki, 0x140271030, PvGameTarget* target)
{
	TargetStateEx* ex = GetTargetStateEx(target->target_index);
	if (IsLongNote(target->target_type) && ex != nullptr)
	{
		DrawLongNoteKiseki(ex);
		return;
	}

	originalDrawKiseki(target);
};

HOOK(void, __fastcall, DrawArcadeGame, 0x140271AB0, PVGameArcade* data)
{
	for (int i = 0; i < state.start_target_count; i++)
	{
		TargetStateEx* ex = state.start_targets_ex[i];
		if (ex->holding)
		{
			if (ShouldUpdateTargets())
				UpdateLongNoteKiseki(data, nullptr, ex, 1.0f / 60.0f);
			DrawLongNoteKiseki(ex);
		}
	}

	originalDrawArcadeGame(data);
}

void InstallTargetHooks()
{
	INSTALL_HOOK(UpdateKiseki);
	INSTALL_HOOK(DrawKiseki);
	INSTALL_HOOK(DrawArcadeGame);
	INSTALL_HOOK(PVGameReset);
}