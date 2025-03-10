#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <nc_state.h>
#include <nc_log.h>
#include "target.h"
#include "hit_state.h"

const float NormalWindow[5]  = { 0.03f, 0.07f, 0.1f, 0.13f };
const float StarWindow[5]    = { 0.03f, 0.07f, 0.1f, 0.13f }; // TODO: CHANGE

namespace nc
{
	static bool CheckTiming(float time, int32_t hit_state, bool star)
	{
		float early = star ? StarWindow[hit_state] : NormalWindow[hit_state];
		float late = -early;
		return time >= late && time <= early;
	}

	static int32_t JudgeTiming(float time, bool star, bool wrong)
	{
		if (CheckTiming(time, HitState_Cool, star))
			return wrong ? HitState_WrongCool : HitState_Cool;
		else if (CheckTiming(time, HitState_Fine, star))
			return wrong ? HitState_WrongFine : HitState_Fine;
		else if (CheckTiming(time, HitState_Safe, star))
			return wrong ? HitState_WrongSafe : HitState_Safe;
		else if (CheckTiming(time, HitState_Sad, star))
			return wrong ? HitState_WrongSad : HitState_Sad;

		return HitState_None;
	}

	static int32_t GetHitStateInternal(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex, ButtonState** hold_button, bool* double_tapped)
	{
		if (ex->force_hit_state != HitState_None)
			return ex->force_hit_state;

		bool hit = false;
		bool wrong = false;

		// NOTE: Check input for double notes
		if (target->target_type >= TargetType_UpW && target->target_type <= TargetType_LeftW)
		{
			int32_t base_index = target->target_type - TargetType_UpW;
			ButtonState* face = &macro_state.buttons[Button_Triangle + base_index];
			ButtonState* arrow = &macro_state.buttons[Button_Up + base_index];

			*double_tapped = (face->IsTapped() && arrow->IsTappedInNearFrames()) ||
				(arrow->IsTapped() && face->IsTappedInNearFrames());
			hit = (face->IsDown() && arrow->IsTapped()) || (arrow->IsDown() && face->IsTapped());
			

			// TODO: Add logic for WRONG
			//
			//
		}
		else if (target->target_type >= TargetType_TriangleLong && target->target_type <= TargetType_SquareLong)
		{
			int32_t base_index = target->target_type - TargetType_TriangleLong;
			ButtonState* face = &macro_state.buttons[Button_Triangle + base_index];
			ButtonState* arrow = &macro_state.buttons[Button_Up + base_index];

			if (!ex->long_end)
			{
				hit = face->IsTapped() || arrow->IsTapped();
				*hold_button = face->IsTapped() ? face : arrow;
			}
			else if (ex->long_end)
			{
				if (ex->prev->hold_button != nullptr)
					hit = ex->prev->hold_button->IsReleased();
			}

			// TODO: Add logic for WRONG
			//
		}
		else if (ex->IsStarLikeNote())
			hit = macro_state.GetStarHit();
		else if (target->target_type == TargetType_StarW)
			hit = macro_state.GetDoubleStarHit(double_tapped);

		if (target->flying_time_remaining < -NormalWindow[HitState_Sad])
			return HitState_Worst;
		
		if (hit)
			return nc::JudgeTiming(target->flying_time_remaining, ex->IsStarLikeNote(), wrong);

		return HitState_None;
	}
}

bool nc::CheckHit(int32_t hit_state, bool wrong, bool worst)
{
	bool cond = (hit_state >= HitState_Cool && hit_state <= HitState_Sad);
	cond |= (hit_state >= HitState_CoolDouble && hit_state <= HitState_SadQuad);
	if (wrong)
		cond |= (hit_state >= HitState_WrongCool && hit_state <= HitState_WrongSad);
	if (worst)
		cond |= (hit_state == HitState_Worst);
	return cond;
}

int32_t nc::JudgeNoteHit(PVGameArcade* game, PvGameTarget** group, TargetStateEx** extras, int32_t group_count, bool* success)
{
	if (group_count < 1)
		return HitState_None;

	for (int i = 0; i < group_count; i++)
	{
		PvGameTarget* target = group[i];
		TargetStateEx* ex = extras[i];

		// NOTE: Evaluate note hit
		ButtonState* hold_button = nullptr;
		bool double_tapped = false;
		int32_t hit_state = nc::GetHitStateInternal(game, target, ex, &hold_button, &double_tapped);

		if (hit_state != HitState_None)
		{
			if (CheckHit(hit_state, false, false))
			{
				if (ex->IsLongNoteStart())
				{
					ex->hold_button = hold_button;
					ex->holding = true;
					ex->next->force_hit_state = HitState_None;
				}
				else if (target->target_type == TargetType_ChanceStar)
					*success = state.chance_time.GetFillRate() == 15;
			}

			target->hit_state = hit_state;
			ex->hit_state = hit_state;
			ex->double_tapped = double_tapped;
			target->hit_time = target->flying_time_remaining;
		}
	}

	return extras[0]->hit_state;
}

bool nc::CheckLongNoteHolding(TargetStateEx* ex)
{
	if (ex->hold_button == nullptr)
		return false;

	ex->holding = ex->hold_button->IsDown();
	return ex->holding;
}