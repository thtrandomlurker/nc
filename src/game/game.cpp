#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <hooks.h>
#include <nc_state.h>
#include <nc_log.h>
#include <save_data.h>
#include "target.h"
#include "chance_time.h"
#include "hit_state.h"

constexpr int32_t DoubleScoreBonus = 200;
constexpr float   LongNoteInterval = 0.1;
constexpr int32_t RushNotePopBonus = 30;
constexpr int32_t ChanceTimeScoreBonus[4]   = { 1000, 600, 200, 60 };
constexpr int32_t LongNoteHoldScoreBonus[4] = { 20,   10,  10,  0  };
constexpr int32_t LinkNoteScoreBonus[4]     = { 200,  100, 0,   0  };

static void SetNoteScoreBonus(TargetStateEx* ex)
{
	if (ex->hit_state < HitState_Cool || ex->hit_state > HitState_Sad)
		return;

	if (ex->double_tapped)
		ex->score_bonus += 200;

	if (ex->IsLinkNote())
		ex->score_bonus += LinkNoteScoreBonus[ex->hit_state];

	if (state.chance_time.CheckTargetInRange(ex->target_index))
		ex->ct_score_bonus += ChanceTimeScoreBonus[ex->hit_state];

	int32_t final_bonus = ex->score_bonus + ex->ct_score_bonus;
	int32_t final_bonus_disp = final_bonus;

	if (ex->IsLongNoteEnd())
	{
		final_bonus += ex->prev->score_bonus;
		final_bonus_disp += ex->prev->score_bonus + ex->prev->ct_score_bonus;
	}
	else if (ex->IsLinkNote())
	{
		for (TargetStateEx* prev = ex->prev; prev != nullptr; prev = prev->prev)
			final_bonus_disp += prev->ct_score_bonus + prev->score_bonus;
	}

	if (final_bonus_disp > 0)
		GetPVGameData()->ui.SetBonusText(final_bonus_disp, ex->target_pos.x, ex->target_pos.y);

	if (final_bonus > 0)
		GetPVGameData()->score += final_bonus;
};

static void CalculateLongNoteScoreBonus(TargetStateEx* ex)
{
	if (ex->holding && ex->hit_state >= HitState_Cool && ex->hit_state <= HitState_Sad)
	{
		if (ex->long_bonus_timer >= LongNoteInterval)
		{
			ex->score_bonus += LongNoteHoldScoreBonus[ex->hit_state];
			ex->long_bonus_timer -= LongNoteInterval;
		}

		if (ex->score_bonus > 0 || ex->ct_score_bonus > 0)
		{
			GetPVGameData()->ui.SetBonusText(
				ex->score_bonus + ex->ct_score_bonus,
				ex->target_pos.x,
				ex->target_pos.y
			);
		}
	}
}

static void IncreaseRushNoteScoreBonus(TargetStateEx* ex)
{
	ex->score_bonus += RushNotePopBonus;
	ex->bal_hit_count += 1;
	
	if (ex->bal_hit_count <= ex->bal_max_hit_count)
		ex->bal_scale += 1.0f / static_cast<float>(ex->bal_max_hit_count);

	GetPVGameData()->score += RushNotePopBonus;
	if (ex->score_bonus >= 0)
		GetPVGameData()->ui.SetBonusText(ex->score_bonus, ex->target_pos.x, ex->target_pos.y);

	diva::vec2 pos = GetScaledPosition(ex->target_pos);
	state.PlayRushHitEffect(pos, 0.6f * (1.0f + ex->bal_scale), false);
}

HOOK(int32_t, __fastcall, GetHitState, 0x14026BF60,
	PVGameArcade* game,
	bool* play_default_se,
	size_t* rating_count,
	diva::vec2* rating_pos,
	int32_t* a5,
	SoundEffect* se,
	int32_t* multi_count,
	float* player_hit_time,
	int32_t* target_index,
	bool* is_success_note,
	bool* slide,
	bool* slide_chain,
	bool* slide_chain_start,
	bool* slide_chain_max,
	bool* slide_chain_continues,
	void* a16)
{
	int32_t final_hit_state = HitState_None;

	// NOTE: Update input manager
	macro_state.Update(game->ptr08, 0);
	state.sound_effects.SetSoundEffects(*se);

	if (ShouldUpdateTargets())
	{
		for (TargetStateEx* tgt : state.target_references)
		{
			// NOTE: Poll input for ongoing long notes
			if (tgt->IsLongNoteStart() && tgt->holding)
			{
				bool is_in_zone = false;

				// NOTE: Check if the end target is in it's timing window
				if (tgt->next->org != nullptr)
				{
					float time = tgt->next->org->flying_time_remaining;
					is_in_zone = time >= game->sad_late_window && time <= game->sad_early_window;
				}

				CalculateLongNoteScoreBonus(tgt);

				// NOTE: Check if the start target button has been released;
				//       if it's the end note is not inside it's timing zone,
				//       automatically mark it as a fail.
				if (!nc::CheckLongNoteHolding(tgt) && !is_in_zone)
				{
					tgt->next->force_hit_state = HitState_Worst;
					tgt->StopAet();
					tgt->holding = false;
					state.PopTarget(tgt);
					state.PlaySoundEffect(SEType_LongFail);
				}
			}
			// NOTE: Poll input for ongoing rush notes
			else if (tgt->IsRushNote() && tgt->holding)
			{
				if (nc::CheckRushNotePops(tgt))
				{
					IncreaseRushNoteScoreBonus(tgt);

					if (tgt->target_type == TargetType_StarRush)
					{
						state.PlaySoundEffect(SEType_Star);
						game->mute_slide_chime = true;
					}
				}
			}
		}
	}

	// NOTE: Determine which targets to be updated and what type are them (custom or vanilla)
	PvGameTarget* group[4] = { nullptr, nullptr, nullptr, nullptr };
	int32_t group_count = 0;
	bool has_custom_note = false;
	bool has_vanilla_note = false;

	for (PvGameTarget* target = game->target; target != nullptr; target = target->next)
	{
		if (group_count < 4)
		{
			group[group_count++] = target;
			has_custom_note |= target->target_type >= TargetType_Custom;
			has_vanilla_note |= target->target_type < TargetType_Custom;
		}

		if (target->multi_count == -1 || target->multi_count != game->target->multi_count)
			break;
	}

	// NOTE: Issue a warning if there's a multi mixing custom and vanilla notes,
	//       as that is not currently supported.
	if (has_custom_note && has_vanilla_note)
	{
		nc::Print("Target #%d is a multi note that mixes Vanilla and custom notes. That is not supported and might cause some issues.", group[0]->target_index);
	}

	if (has_vanilla_note || !ShouldUpdateTargets() || group_count < 1)
	{
		final_hit_state = originalGetHitState(
			game,
			play_default_se,
			rating_count,
			rating_pos,
			a5,
			se,
			multi_count,
			player_hit_time,
			target_index,
			is_success_note,
			slide,
			slide_chain,
			slide_chain_start,
			slide_chain_max,
			slide_chain_continues,
			a16
		);
	}
	else if (has_custom_note)
	{
		// NOTE: Set default return values
		*is_success_note = false;
		*slide = false;
		*slide_chain = false;
		*slide_chain_start = false;
		*slide_chain_max = false;
		*slide_chain_continues = false;
		bool success = false;

		// NOTE: Get extra data for the targets
		TargetStateEx* extras[4] = { nullptr, nullptr, nullptr, nullptr };
		for (int i = 0; i < group_count; i++)
			extras[i] = GetTargetStateEx(group[i]);

		// NOTE: Judge hit and set post-hit information
		int32_t hit_state = nc::JudgeNoteHit(game, group, extras, group_count, &success);
		if (hit_state != HitState_None)
		{
			final_hit_state = hit_state;
			*multi_count = group_count;
			*target_index = group[0]->target_index;
			*player_hit_time = group[0]->player_hit_time;
			
			if (success)
				GetPVGameData()->is_success_branch = true;

			for (int i = 0; i < group_count; i++)
			{
				PvGameTarget* tgt = group[i];
				TargetStateEx* ex = extras[i];

				if (ex->IsLongNoteStart())
				{
					if (ex->IsWrong())
					{
						state.PopTarget(ex);
						ex->StopAet();
						ex->next->force_hit_state = ex->hit_state;
					}
					else
						ex->SetLongNoteAet();
				}
				else if (ex->IsRushNote())
				{
					if (!ex->IsWrong())
					{
						ex->SetRushNoteAet();
						state.PlaySoundEffect(SEType_RushStart);
					}
					else
						state.PopTarget(ex);
				}
				else if (ex->IsLongNoteEnd())
				{
					ex->prev->StopAet();
					state.PopTarget(ex->prev);
				}
				else if (ex->IsLinkNoteEnd() && nc::CheckHit(hit_state, true, false))
				{
					// NOTE: Find chain start target
					TargetStateEx* chain = nullptr;
					for (chain = ex; chain != nullptr && chain->prev != nullptr; chain = chain->prev) { }

					if (chain != nullptr)
						chain->StopAet();
					ex->StopAet();
				}

				rating_pos[(*rating_count)++] = tgt->target_pos;

				game->EraseTarget(tgt);
				game->RemoveTargetAet(tgt);

				// NOTE: Play note hit effect
				//
				int32_t hit_effect_index = -1;
				switch (ex->hit_state)
				{
				case HitState_Cool:
					hit_effect_index = 1;
					break;
				case HitState_Fine:
					hit_effect_index = 2;
					break;
				case HitState_Safe:
					hit_effect_index = 3;
					break;
				case HitState_Sad:
					hit_effect_index = 4;
					break;
				case HitState_WrongCool:
				case HitState_WrongFine:
				case HitState_WrongSafe:
				case HitState_WrongSad:
					hit_effect_index = 0;
					break;
				}

				if (hit_effect_index != -1)
					game->PlayHitEffect(hit_effect_index, tgt->target_pos);

				// NOTE: Play note SE
				//
				if (ex->hit_state != HitState_Worst)
				{
					switch (ex->target_type)
					{
					case TargetType_TriangleRush:
					case TargetType_CircleRush:
					case TargetType_CrossRush:
					case TargetType_SquareRush:
						sound::PlaySoundEffect(3, se->button.c_str(), 1.0f);
						break;
					case TargetType_UpW:
					case TargetType_RightW:
					case TargetType_DownW:
					case TargetType_LeftW:
						state.PlaySoundEffect(SEType_Double);
						break;
					case TargetType_TriangleLong:
					case TargetType_CircleLong:
					case TargetType_CrossLong:
					case TargetType_SquareLong:
						state.PlaySoundEffect(ex->IsLongNoteStart() ? SEType_LongStart : SEType_LongRelease);
						break;
					case TargetType_LinkStar:
						if (ex->IsLinkNoteStart()) { state.PlaySoundEffect(SEType_LinkStart); }
						[[fallthrough]];
					case TargetType_Star:
					case TargetType_StarRush:
					case TargetType_LinkStarEnd:
						state.PlaySoundEffect(SEType_Star);
						game->mute_slide_chime = true;
						break;
					case TargetType_ChanceStar:
						if (state.chance_time.GetFillRate() == 15)
							state.PlaySoundEffect(SEType_Cymbal);
						else
							state.PlaySoundEffect(SEType_Star);
						break;
					case TargetType_StarW:
						state.PlaySoundEffect(SEType_StarDouble);
						break;
					}

					*play_default_se = false;
				}
				else if (ex->IsLongNoteEnd())
					state.PlaySoundEffect(SEType_LongFail);

				if (ex->IsLinkNoteEnd())
					state.PlaySoundEffect(SEType_LinkEnd);
			}
		}
	}
	
	if (nc::CheckHit(final_hit_state, false, false))
	{
		// NOTE: Calculate score bonus
		for (int i = 0; i < group_count; i++)
		{
			TargetStateEx* ex = GetTargetStateEx(group[i]);
			ex->hit_state = group[i]->hit_state;
			SetNoteScoreBonus(ex);
		}

		// NOTE: Update chance time fill rate
		if (state.chance_time.CheckTargetInRange(*target_index))
			state.chance_time.targets_hit += 1;
	}

	if (macro_state.GetStarHit() && *play_default_se && nc::GetSharedData().stick_control_se == 1 && state.GetGameStyle() != GameStyle_Arcade)
	{
		state.PlaySoundEffect(SEType_Star);
		game->mute_slide_chime = true;
	}

	return final_hit_state;
}

static float GetPercentageF2nd(const PVGameData* pv_game)
{
	int32_t target_count = static_cast<int32_t>(pv_game->pv_data.targets.size());
	int32_t correct_hits = pv_game->judge_count_correct[BasicHitState_Cool] + pv_game->judge_count_correct[BasicHitState_Fine];

	float targets_ratio = static_cast<float>(correct_hits) / static_cast<float>(target_count);
	float completion_rate = targets_ratio * state.scoring_info.target_max_rate;

	if (state.chance_time.IsValid() && state.chance_time.successful)
		completion_rate += ChanceTimeRetainedRate;

	return completion_rate * 100.0f;
}

HOOK(void, __fastcall, CalculatePercentage, 0x140246130, PVGameData* pv_game)
{
	switch (state.GetScoreMode())
	{
	case ScoreMode_F2nd:
		pv_game->percentage = GetPercentageF2nd(pv_game);
		break;
	default:
		originalCalculatePercentage(pv_game);
		break;
	}
}

HOOK(void, __fastcall, UpdateLife, 0x140245220, PVGameData* a1, int32_t hit_state, bool a3, bool is_challenge_time, int32_t a5, bool a6, bool a7, bool a8)
{
	originalUpdateLife(
		a1,
		hit_state,
		a3,
		is_challenge_time || state.chance_time.enabled,
		a5,
		a6,
		a7,
		a8
	);
}

HOOK(void, __fastcall, ExecuteModeSelect, 0x1503B04A0, PVGamePvData* pv_data, int32_t op)
{
	if (!game::IsPvMode())
	{
		int32_t op_difficulty = pv_data->script_buffer[pv_data->script_pos + 1];
		int32_t difficulty = 1 << GetPvGameplayInfo()->difficulty;
		if ((op_difficulty & difficulty) != 0)
			SetChanceTimeMode(&pv_data->pv_game->ui, pv_data->script_buffer[pv_data->script_pos + 2]);
	}

	return originalExecuteModeSelect(pv_data, op);
}

HOOK(void, __fastcall, UpdateGaugeFrame, 0x14027A490, PVGameUI* ui)
{
	originalUpdateGaugeFrame(ui);
	if (state.chance_time.enabled)
		SetChanceTimeStarFill(ui, state.chance_time.GetFillRate());
	SetChanceTimePosition(ui);
}

void InstallGameHooks()
{
	INSTALL_HOOK(GetHitState);
	INSTALL_HOOK(UpdateLife);
	INSTALL_HOOK(ExecuteModeSelect);
	INSTALL_HOOK(UpdateGaugeFrame);
	INSTALL_HOOK(CalculatePercentage);
}