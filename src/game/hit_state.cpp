#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <common.h>
#include "target.h"
#include "hit_state.h"

static FUNCTION_PTR(void, __fastcall, sub_14026EC80, 0x14026EC80, PVGameArcade* data);
static FUNCTION_PTR(void, __fastcall, EraseTarget, 0x14026E5C0, PVGameArcade* data, PvGameTarget* target);
static FUNCTION_PTR(void, __fastcall, FinishTargetAet, 0x14026E640, PVGameArcade* data, PvGameTarget* target);
static FUNCTION_PTR(void, __fastcall, PlayNoteHitEffect, 0x1402715F0, PVGameArcade* data, int32_t effect, diva::vec2* pos);

static bool CheckHit(int32_t hit_state, bool wrong, bool worst)
{
	bool cond = (hit_state >= HitState_Cool && hit_state <= HitState_Sad);
	if (wrong)
		cond |= (hit_state >= HitState_WrongCool && hit_state <= HitState_WrongSad);
	if (worst)
		cond |= (hit_state == HitState_Worst);
	return cond;
}

static void FinishExtraAet(TargetStateEx* ex)
{
	if (ex == nullptr)
		return;

	diva::aet::Stop(&ex->target_aet);
	ex->kiseki.clear();
	ex->vertex_count_max = 0;
}

static void FinishAetButCopyTarget(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex)
{
	if (data->play)
	{
		sub_14026EC80(data);
		diva::aet::Stop(&target->button_aet);
		diva::aet::Stop(&target->dword78);
		diva::aet::Stop(&target->target_eff_aet);

		ex->target_aet = target->target_aet;
		target->target_aet = 0;
		diva::aet::SetFrame(ex->target_aet, 360.0f);
		// TODO: Set scale to 1.0;
		//       Fix KISEKI pos when hit after the cool window!!!
		//
		diva::aet::SetPlay(ex->target_aet, false);
	}
}

static int32_t GetHitStateNC(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex)
{
	if (IsLongNote(target->target_type) && ex == nullptr)
		return HitState_None;

	bool hit = false;
	bool wrong = false;
	int32_t hit_state = HitState_None;
	ButtonState* button = nullptr;

	// NOTE: Check input for double notes
	if (target->target_type >= TargetType_UpW && target->target_type <= TargetType_LeftW)
	{
		int32_t base_index = target->target_type - TargetType_UpW;
		ButtonState* face = &input_state.buttons[Button_Triangle + base_index];
		ButtonState* arrow = &input_state.buttons[Button_Up + base_index];

		if (face->tapped && arrow->tapped)
		{
			// TODO: Set score bonus
		}

		hit = (face->down && arrow->tapped) || (arrow->down && face->tapped);

		// TODO: Add logic for WRONG
		//
		//
	}
	else if (target->target_type >= TargetType_TriangleLong && target->target_type <= TargetType_SquareLong)
	{
		int32_t base_index = target->target_type - TargetType_TriangleLong;
		ButtonState* face = &input_state.buttons[Button_Triangle + base_index];
		ButtonState* arrow = &input_state.buttons[Button_Up + base_index];

		if (ex != nullptr && !ex->end)
		{
			hit = face->tapped || arrow->tapped;
			button = face->tapped ? face : arrow;
		}
		else if (ex != nullptr && ex->end)
		{
			if (state.hold_button != nullptr)
				hit = state.hold_button->released;
		}
	}
	else if (target->target_type == TargetType_Star || target->target_type == TargetType_ChanceStar)
	{
		for (int i = Button_L1; i <= Button_R2; i++)
			hit |= input_state.buttons[i].tapped;
	}
	else if (target->target_type == TargetType_StarW)
	{
		ButtonState* left[2] = {
			&input_state.buttons[Button_L1],
			&input_state.buttons[Button_L1]
		};

		ButtonState* right[2] = {
			&input_state.buttons[Button_R1],
			&input_state.buttons[Button_R2]
		};

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				if (left[i]->tapped && right[j]->tapped)
				{
					// TODO: Set score bonus
				}

				hit = (left[i]->down && right[j]->tapped) || (right[j]->down && left[i]->tapped);
				if (hit) break;
			}

			if (hit) break;
		}
	}

	if (hit)
	{
		float time = target->flying_time_remaining;

		if (time >= data->cool_late_window && time <= data->cool_early_window)
			hit_state = wrong ? HitState_WrongCool : HitState_Cool;
		else if (time >= data->fine_late_window && time <= data->fine_early_window)
			hit_state = wrong ? HitState_WrongFine : HitState_Fine;
		else if (time >= data->safe_late_window && time <= data->safe_early_window)
			hit_state = wrong ? HitState_WrongSafe : HitState_Safe;
		else if (time >= data->sad_late_window && time <= data->sad_early_window)
			hit_state = wrong ? HitState_WrongSad : HitState_Sad;

		// NOTE: Handle long note input
		if (IsLongNote(target->target_type) && ex != nullptr)
		{
			if (CheckHit(hit_state, false, false) && !ex->end)
			{
				state.start_target_ex = ex;
				ex->kiseki_pos = target->target_pos;
				ex->kiseki_dir = target->delta_pos_sq;
				ex->holding = true;
				// NOTE: This means that there should be no targets between this and
				//       the end target. Should I change that? I feel like there could be a better
				//       way to do this.
				state.end_target_ex = GetTargetStateEx(target->target_index + 1);
				state.hold_button = button;
				state.next_end_hit_state = HitState_None;
			}
		}
	}

	if (target->flying_time_remaining < data->sad_late_window)
	{
		hit_state = HitState_Worst;
		if (IsLongNote(target->target_type) && ex != nullptr && !ex->end)
			state.next_end_hit_state = HitState_Worst;
	}

	return hit_state;
}

static int32_t UpdateHitStateLongNC(TargetStateEx* target)
{
	if (state.hold_button->down)
		return HitState_None;

	diva::aet::Stop(&target->target_aet);
	target->holding = false;
	target->se_state = SEState_FailRelease;
	PlayUpdateSoundEffect(nullptr, target, nullptr);
	return HitState_Worst;
};

HOOK(int32_t, __fastcall, GetHitState, 0x14026BF60, PVGameArcade* data, bool* play_default_se, size_t* rating_count, diva::vec2* rating_pos, int32_t a5, SoundEffect* se, int32_t* multi_count, int32_t* a8, int32_t* a9, bool* a10, bool* slide, bool* slide_chain, bool* slide_chain_start, bool* slide_chain_max, bool* slide_chain_continues)
{
	if (!data->play)
		return HitState_None;

	// NOTE: This being right here is likely being called framely, if not I have to move this
	//       somewhere else...
	GetCurrentInputState(data->ptr08, &input_state);

	// NOTE: Poll input for ongoing long note
	//
	if (ShouldUpdateTargets())
	{
		if (state.start_target_ex != nullptr && state.end_target_ex != nullptr)
		{
			// NOTE: Check if the next target is available
			bool is_in_zone = false;
			if (data->target != nullptr)
			{
				is_in_zone = CheckWindow(
					data->target->flying_time_remaining,
					data->sad_early_window,
					data->sad_late_window
				);
			}

			int32_t long_state = UpdateHitStateLongNC(state.start_target_ex);
			if (long_state == HitState_Worst && !is_in_zone)
			{
				// NOTE: Remove references and set future data for the long end note
				//
				state.next_end_hit_state = HitState_Worst;
				state.start_target_ex = nullptr;
				state.end_target_ex = nullptr;

				return HitState_None;
			}
		}
	}

	if (data->target == nullptr)
		return HitState_None;

	if (data->target->target_type < TargetType_Custom)
	{
		return originalGetHitState(data, play_default_se, rating_count, rating_pos, a5, se, multi_count, a8, a9, a10, slide, slide_chain, slide_chain_start, slide_chain_max, slide_chain_continues);
	}

	// NOTE: Set default return values
	//
	*play_default_se = true;
	*rating_count = 0;
	*multi_count = 0;
	*slide = false;
	*slide_chain = false;
	*slide_chain_start = false;
	*slide_chain_max = false;
	*slide_chain_continues = false;

	if (rating_count != nullptr)
		*rating_count = 0;

	int32_t hit_state = HitState_None;
	int32_t target_count = 0;
	PvGameTarget* targets[4] = { };

	// TODO: Add support for multi notes? Is that something we wanna do?
	if (data->target != nullptr)
	{
		targets[0] = data->target;
		target_count = 1;
	}

	for (int i = 0; i < target_count; i++)
	{
		PvGameTarget* target = targets[i];
		TargetStateEx* extra = GetTargetStateEx(target->target_index);

		if (IsLongNote(target->target_type) && extra != nullptr)
		{
			if (extra->end && state.next_end_hit_state != HitState_None)
			{
				hit_state = state.next_end_hit_state;
				target->hit_state = hit_state;

				// NOTE: Hide the combo counter
				*rating_count = 1;
				rating_pos[0] = { -1200.0f, 0.0f };

				FinishTargetAet(data, target);
				continue;
			}
		}

		hit_state = GetHitStateNC(data, target, extra);
		target->hit_state = hit_state;

		if (hit_state != HitState_None)
		{
			// NOTE: Play hit effect Aet
			//
			int32_t eff_index = -1;
			switch (hit_state)
			{
			case HitState_Cool:
				eff_index = 1;
				break;
			case HitState_Fine:
				eff_index = 2;
				break;
			case HitState_Safe:
				eff_index = 3;
				break;
			case HitState_Sad:
				eff_index = 4;
				break;
			case HitState_WrongCool:
			case HitState_WrongFine:
			case HitState_WrongSafe:
			case HitState_WrongSad:
				eff_index = 0;
				break;
			};

			if (eff_index != -1)
				PlayNoteHitEffect(data, eff_index, &target->target_pos);

			// NOTE: Play hit sound effect
			if (CheckHit(hit_state, true, false))
			{
				if (IsLongNote(target->target_type) && extra != nullptr && extra->end)
				{
					state.start_target_ex->se_state = SEState_SuccessRelease;
					PlayUpdateSoundEffect(nullptr, state.start_target_ex, nullptr);
				}
				else
					PlayUpdateSoundEffect(target, extra, se);
				*play_default_se = false;
			}

			// NOTE: Remove target aet objects
			//
			if (extra != nullptr)
			{
				if (CheckHit(hit_state, false, false) && IsLongNote(target->target_type) && !extra->end)
					FinishAetButCopyTarget(data, target, extra);
				else if (IsLongNote(target->target_type) && extra->end)
				{
					FinishExtraAet(state.start_target_ex);
					FinishExtraAet(extra);
					state.start_target_ex = nullptr;
					state.end_target_ex = nullptr;
				}
				else
					FinishTargetAet(data, target);
			}
			else
				FinishTargetAet(data, target);

			// NOTE: Set position of the combo counter
			//
			if (rating_count != nullptr && rating_pos != nullptr)
				rating_pos[(*rating_count)++] = target->target_pos;
		}
	}

	// NOTE: Erase passed targets from list
	for (int i = 0; i < target_count; i++)
	{
		if (targets[i]->hit_state != HitState_None)
			EraseTarget(data, targets[i]);
	}

	*multi_count = target_count;
	return hit_state;
}

/*
HOOK(int32_t, __fastcall, GetHitStateInternal, 0x14026D2E0, PVGameArcade* data, PvGameTarget* target, uint16_t keys, uint16_t a4)
{
	// NOTE: Call the ordinary routine for vanilla targets
	if (target->target_type < TargetType_Custom || target->target_type >= TargetType_Max)
		return originalGetHitStateInternal(data, target, keys, a4);

	if (target->hit_state != HitState_None)
		return target->hit_state;

	// NOTE: Do our custom note's handling
	
};
*/

bool InstallHitStateHooks()
{
	INSTALL_HOOK(GetHitState);
	return true;
}