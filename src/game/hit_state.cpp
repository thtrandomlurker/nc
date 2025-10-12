#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <hooks.h>
#include <nc_state.h>
#include <nc_log.h>
#include "target.h"
#include "hit_state.h"

const float NormalWindow[5]  = { 0.03f, 0.07f, 0.1f,  0.13f };
const float StarWindow[5]    = { 0.03f, 0.12f, 0.13f, 0.13f };

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

	static int32_t GetHitStateInternal(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex, ButtonState** hold_button, bool* double_tapped, bool* no_success)
	{
		if (ex->force_hit_state != HitState_None)
			return ex->force_hit_state;

		if (target->flying_time_remaining < -NormalWindow[HitState_Sad])
			return HitState_Worst;

		bool hit = false;
		bool wrong = false;

		uint64_t button_mask = 0;
		uint64_t wrong_mask = 0;
		switch (target->target_type)
		{
		case TargetType_UpW:
		case TargetType_TriangleLong:
		case TargetType_TriangleRush:
			button_mask = GetButtonMask(Button_Triangle) | GetButtonMask(Button_Up);
			wrong_mask = ~button_mask & GetButtonMask(Button_Max);
			break;
		case TargetType_RightW:
		case TargetType_CircleLong:
		case TargetType_CircleRush:
			button_mask = GetButtonMask(Button_Circle) | GetButtonMask(Button_Right);
			wrong_mask = ~button_mask & GetButtonMask(Button_Max);
			break;
		case TargetType_DownW:
		case TargetType_CrossLong:
		case TargetType_CrossRush:
			button_mask = GetButtonMask(Button_Cross) | GetButtonMask(Button_Down);
			wrong_mask = ~button_mask & GetButtonMask(Button_Max);
			break;
		case TargetType_LeftW:
		case TargetType_SquareLong:
		case TargetType_SquareRush:
			button_mask = GetButtonMask(Button_Square) | GetButtonMask(Button_Left);
			wrong_mask = ~button_mask & GetButtonMask(Button_Max);
			break;
		}

		// NOTE: Check input for double notes
		if (target->target_type >= TargetType_UpW && target->target_type <= TargetType_LeftW)
		{
			ButtonState& face = macro_state.buttons[Button_Triangle + (target->target_type - TargetType_UpW)];
			ButtonState& arrow = macro_state.buttons[Button_Up + (target->target_type - TargetType_UpW)];

			if ((face.IsTapped() && arrow.IsDown()) || (arrow.IsTapped() && face.IsDown()))
			{
				hit = true;
				wrong = false;
				*double_tapped = (face.IsTapped() && arrow.IsTappedInNearFrames()) ||
					(arrow.IsTapped() && face.IsTappedInNearFrames());
			}
			else if ((macro_state.GetTappedBitfield() & wrong_mask) != 0)
			{
				hit = true;
				wrong = true;
			}
		}
		else if (target->target_type >= TargetType_TriangleLong && target->target_type <= TargetType_SquareLong)
		{
			ButtonState& face = macro_state.buttons[Button_Triangle + (target->target_type - TargetType_TriangleLong)];
			ButtonState& arrow = macro_state.buttons[Button_Up + (target->target_type - TargetType_TriangleLong)];

			if (!ex->long_end)
			{
				if (face.IsTapped() || arrow.IsTapped())
				{
					hit = true;
					wrong = false;
					*hold_button = face.IsTapped() ? &face : &arrow;
				}
				else if ((macro_state.GetTappedBitfield() & wrong_mask) != 0)
				{
					hit = true;
					wrong = true;
				}
			}
			else
			{
				if (ex->prev->hold_button != nullptr)
				{
					hit = ex->prev->hold_button->IsReleased();
					wrong = false;
				}
			}
		}
		else if (ex->IsStarLikeNote())
		{
			hit = macro_state.GetStarHit();
			if (target->target_type == TargetType_ChanceStar)
				*no_success = macro_state.GetStarHitCancel();
		}
		else if (target->target_type == TargetType_StarW)
			hit = macro_state.GetDoubleStarHit();
		else if (ex->IsRushNote())
		{
			if ((macro_state.GetTappedBitfield() & button_mask) != 0)
			{
				hit = true;
				wrong = false;
			}
			else if ((macro_state.GetTappedBitfield() & wrong_mask) != 0)
			{
				hit = true;
				wrong = true;
			}
		}
		
		if (hit)
			return nc::JudgeTiming(target->flying_time_remaining, ex->IsStarLikeNote() || ex->target_type == TargetType_StarW, wrong);

		return HitState_None;
	}
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
		bool no_success = false;
		int32_t hit_state = nc::GetHitStateInternal(game, target, ex, &hold_button, &double_tapped, &no_success);

		if (hit_state != HitState_None)
		{
			if (nc::IsHitCorrect(hit_state))
			{
				if (ex->IsLongNoteStart())
				{
					ex->hold_button = hold_button;
					ex->holding = true;
					ex->fix_long_kiseki = true;
					ex->kiseki_pos = ex->target_pos;
					ex->next->force_hit_state = HitState_None;
				}
				else if (ex->IsRushNote())
					ex->holding = true;
				else if (target->target_type == TargetType_ChanceStar)
				{
					if (nc::IsHitGreat(hit_state) && state.chance_time.GetFillRate() == 15)
					{
						*success = true;
						state.chance_time.successful = true;
						state.chance_time.chance_star_hit_time = game->current_time;
					}
				}
			}

			target->hit_state = hit_state;
			target->player_hit_time = target->flying_time_remaining;
			ex->hit_state = hit_state;
			ex->hit_time = target->flying_time_remaining;
			ex->double_tapped = double_tapped;
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

bool nc::CheckRushNotePops(TargetStateEx* ex)
{
	ButtonState* button1 = nullptr;
	ButtonState* button2 = nullptr;

	switch (ex->target_type)
	{
	case TargetType_TriangleRush:
	case TargetType_CircleRush:
	case TargetType_CrossRush:
	case TargetType_SquareRush:
		button1 = &macro_state.buttons[ex->target_type - TargetType_TriangleRush + Button_Triangle];
		button2 = &macro_state.buttons[ex->target_type - TargetType_TriangleRush + Button_Up];
		break;
	case TargetType_StarRush:
		return macro_state.GetStarHit();
	}

	bool cond = false;
	if (button1 != nullptr)
		cond |= button1->IsTapped();

	if (button2 != nullptr)
		cond |= button2->IsTapped();

	return cond;
}