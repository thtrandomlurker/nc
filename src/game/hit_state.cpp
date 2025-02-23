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

static void FinishAetButCopyTargetAndButton(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex)
{
	if (data->play)
	{
		sub_14026EC80(data);
		diva::aet::Stop(&target->dword78);
		diva::aet::Stop(&target->target_eff_aet);

		ex->target_aet = target->target_aet;
		ex->button_aet = target->button_aet;
		diva::aet::SetFrame(ex->button_aet, 360.0f);
		diva::aet::SetPlay(ex->button_aet, false);
		target->target_aet = 0;
		target->button_aet = 0;
	}
}

static int32_t GetHitStateNC(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex)
{
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

		if (!ex->long_end)
		{
			hit = face->tapped || arrow->tapped;
			button = face->tapped ? face : arrow;
		}
		else if (ex->long_end)
		{
			if (ex->prev->hold_button != nullptr)
				hit = ex->prev->hold_button->released;
		}
	}
	else if (IsStarLikeNote(target->target_type))
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
		if (IsLongNote(target->target_type))
		{
			if (CheckHit(hit_state, false, false) && !ex->long_end)
			{
				state.PushTarget(ex);
				ex->kiseki_pos = target->target_pos;
				ex->kiseki_dir = target->delta_pos_sq;
				ex->holding = true;
				ex->hold_button = button;
				ex->next->force_hit_state = HitState_None;
			}
		}
	}

	if (target->flying_time_remaining < data->sad_late_window)
	{
		hit_state = HitState_Worst;
		if (IsLongNote(target->target_type) && !ex->long_end)
			ex->next->force_hit_state = HitState_Worst;
	}

	return hit_state;
}

static bool CheckLongNoteState(TargetStateEx* target)
{
	if (target->hold_button->down)
	{
		target->holding = true;
		return true;
	}

	// TODO: Maybe move this somewhere else (?)
	diva::aet::Stop(&target->target_aet);
	target->holding = false;
	return false;
};

HOOK(int32_t, __fastcall, GetHitState, 0x14026BF60, PVGameArcade* data, bool* play_default_se, size_t* rating_count, diva::vec2* rating_pos, int32_t a5, SoundEffect* se, int32_t* multi_count, int32_t* a8, int32_t* a9, bool* a10, bool* slide, bool* slide_chain, bool* slide_chain_start, bool* slide_chain_max, bool* slide_chain_continues)
{
	if (!data->play)
		return HitState_None;

	// NOTE: This being right here is likely being called framely, if not I have to move this
	//       somewhere else...
	GetCurrentInputState(data->ptr08, &input_state);

	// NOTE: Poll input for ongoing long notes
	//
	if (ShouldUpdateTargets())
	{
		for (TargetStateEx* tgt : state.target_references)
		{
			if (!IsLongNote(tgt->target_type))
				continue;

			bool is_in_zone = false;

			// NOTE: Check if the end target is in it's timing window
			if (tgt->next->org != nullptr)
			{
				is_in_zone = CheckWindow(
					tgt->next->org->flying_time_remaining,
					data->sad_early_window,
					data->sad_late_window
				);
			}

			// NOTE: Check if the start target button has been released;
			//       if it's the end note is not inside it's timing zone,
			//       automatically mark it as a fail.
			if (!CheckLongNoteState(tgt) && !is_in_zone)
			{
				tgt->next->force_hit_state = HitState_Worst;
				tgt->se_state = SEState_FailRelease;
				PlayUpdateSoundEffect(nullptr, tgt, nullptr);
			}
		}
	}

	if (data->target != nullptr && data->target->target_type < TargetType_Custom)
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

	int32_t hit_state = HitState_None;
	int32_t target_count = 0;
	PvGameTarget* targets[4] = { };
	TargetStateEx* extras[4] = { };

	// TODO: Add support for multi notes? Is that something we wanna do?
	if (data->target != nullptr)
	{
		targets[0] = data->target;
		extras[0] = GetTargetStateEx(targets[0]->target_index, 0);
		target_count = 1;
	}

	for (int i = 0; i < target_count; i++)
	{
		PvGameTarget* target = targets[i];
		TargetStateEx* extra = extras[i];

		// TODO: Move this into DetermineTarget?
		//
		/*
		if (extra->org == nullptr)
			extra->org = target;
		*/

		// NOTE: First, check if this note isn't already deemed to be fail
		//       (from missing the long note start target)...
		if (extra->force_hit_state != HitState_None)
		{
			hit_state = extra->force_hit_state;
			target->hit_state = extra->force_hit_state;

			// NOTE: Hide the combo counter
			*rating_count = 1;
			rating_pos[0] = { -1200.0f, 0.0f };

			FinishTargetAet(data, target);
			continue;
		}

		hit_state = GetHitStateNC(data, target, extra);
		target->hit_state = hit_state;
		extra->hit_state = hit_state;

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
				if (IsLongNote(target->target_type) && extra->long_end)
				{
					extra->prev->se_state = SEState_SuccessRelease;
					PlayUpdateSoundEffect(nullptr, extra->prev, nullptr);
				}
				else
					PlayUpdateSoundEffect(target, extra, se);
				*play_default_se = false;
			}

			// NOTE: Remove target aet objects
			//
			if (IsLongNote(target->target_type))
			{
				if (CheckHit(target->hit_state, false, false) && !extra->long_end)
					FinishAetButCopyTarget(data, target, extra);
				else if (extra->long_end)
				{
					FinishExtraAet(extra->prev);
					FinishExtraAet(extra);
				}
			}
			else
				FinishTargetAet(data, target);

			// NOTE: Set position of the combo counter
			//
			if (rating_count != nullptr && rating_pos != nullptr)
				rating_pos[(*rating_count)++] = target->target_pos;
		}

		/*
		if (IsLinkStarNote(targets[i]->target_type, false))
		{
			if (target->flying_time_remaining <= data->cool_late_window && extra->delta_time_max <= 0.0f)
			{
				if (extra->next && extra->next->org)
				{
					extra->delta_time_max = extra->next->org->flying_time_remaining;
					extra->delta_time = extra->delta_time_max;
					printf("%.3f / %.3f\n", extra->delta_time, extra->delta_time_max);
				}
			}
		}
		*/
	}

	// NOTE: This means long notes that start at the same time must also end at the same time
	//       or else the program will eventually overwrite other memory inside the StateEx struct.
	//       I don't really plan on supporting multi notes for any of the custom notes, anyway.
	for (int i = 0; i < target_count; i++)
	{
		if (IsLongNote(targets[i]->target_type) && extras[i]->long_end && targets[i]->hit_state != HitState_None)
			state.PopTarget(extras[i]);
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

bool InstallHitStateHooks()
{
	INSTALL_HOOK(GetHitState);
	return true;
}