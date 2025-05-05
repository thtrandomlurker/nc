#include <stdint.h>
#include <diva.h>
#include <nc_state.h>
#include <util.h>
#include "hit_state.h"
#include "score.h"

constexpr int32_t DoubleTapScoreBonus     = 200;
constexpr int32_t RushNotePopBonus        = 30;
constexpr float   SustainBonusInterval    = 0.1;
constexpr int32_t SustainBonusScore[4]    = { 20,   10,  10,  0 };
constexpr int32_t ChanceTimeScoreBonus[4] = { 1000, 600, 200, 60 };
constexpr int32_t LinkNoteScoreBonus[4]   = { 200,  100, 0,   0 };

constexpr float WrongPercentageWeight = 0.5;
static const float JudgePercentageWeight[5][5] = {
	// COOL  FINE  SAFE  SAD   WORST
	{  1.0f, 0.7f, 0.5f, 0.3f, 0.0f  }, // EASY
	{  1.0f, 0.7f, 0.5f, 0.3f, 0.0f  }, // NORMAL
	{  1.0f, 0.7f, 0.5f, 0.3f, 0.0f  }, // HARD
	{  1.0f, 0.7f, 0.5f, 0.3f, 0.0f  }, // EXTREME / EXTRA-EXTREME
	{  0.0f, 0.0f, 0.0f, 0.0f, 0.0f  }, // ENCORE
};

constexpr float ChanceTimePercBonus  = 0.01; // 1%
constexpr float DoubleTapPercBonus   = 0.02; // 2%
constexpr float SustainHoldPercBonus = 0.01; // 1%

int32_t score::CalculateHitScoreBonus(TargetStateEx* target, int32_t* chance_bonus, int32_t* disp)
{
	int32_t bonus = 0;
	int32_t bonus_disp = 0;

	if (target->double_tapped)
	{
		bonus += DoubleTapScoreBonus;
		state.score.double_tap_bonus += DoubleTapScoreBonus;
	}

	if (state.chance_time.CheckTargetInRange(target->target_index))
	{
		if (chance_bonus)
			*chance_bonus = ChanceTimeScoreBonus[target->hit_state];
		bonus += ChanceTimeScoreBonus[target->hit_state];
		state.score.ct_score_bonus += ChanceTimeScoreBonus[target->hit_state];
	}

	if (target->IsLinkNote())
	{
		bonus += LinkNoteScoreBonus[target->hit_state];
		state.score.link_bonus += LinkNoteScoreBonus[target->hit_state];

		for (TargetStateEx* prev = target->prev; prev != nullptr; prev = prev->prev)
			bonus_disp += prev->ct_score_bonus + prev->score_bonus;
	}
	else if (target->IsLongNoteEnd())
		bonus_disp += target->prev->score_bonus;

	if (disp)
		*disp = bonus_disp + bonus;

	target->score_bonus += bonus;
	return bonus;
}

int32_t score::CalculateSustainBonus(TargetStateEx* target)
{
	if (!nc::IsHitCorrect(target->hit_state))
		return 0;

	int32_t bonus = 0;
	while (target->sustain_bonus_time >= SustainBonusInterval)
	{
		bonus += SustainBonusScore[target->hit_state];
		target->sustain_bonus_time -= SustainBonusInterval;
	}

	target->score_bonus += bonus;
	return bonus;
}

int32_t score::IncreaseRushPopCount(TargetStateEx* target)
{
	target->bal_hit_count += 1;
	target->bal_scale = util::Clamp(target->bal_hit_count / static_cast<float>(target->bal_max_hit_count), 0.0f, 1.0f);
	target->score_bonus += RushNotePopBonus;
	state.score.rush_bonus += RushNotePopBonus;
	return RushNotePopBonus;
}

// NOTE: Calculates target + events (Technical Zones and Chance Time) percentage,
//       which on a Full-Combo All-Cool run should give roughly 100%. Bonus percentage
//       can then be applied externally.
static float CalculateFrankenBasePercentage(
	const int32_t* judge,
	const int32_t* judge_wrong,
	const float* weights,
	const float target_max_rate,
	int32_t note_count
)
{
	float percentage = 0.0;

	// NOTE: Calculate target percentage
	for (int32_t i = 0; i < 4; i++)
	{
		float correct_inc = judge[i] / static_cast<float>(note_count) * target_max_rate;
		float wrong_inc = (judge_wrong[i] - judge[i]) / static_cast<float>(note_count) * target_max_rate;

		percentage += (correct_inc * weights[i]) + (wrong_inc * weights[i] * WrongPercentageWeight);
	}

	return percentage;
}

// NOTE: This function assumes it's being called from the rhythm game so the state is valid
float score::CalculatePercentage(PVGameData* pv_game)
{
	float percentage = CalculateFrankenBasePercentage(
		pv_game->judge_count_correct,
		pv_game->judge_count,
		JudgePercentageWeight[GetPvGameplayInfo()->difficulty],
		state.score.target_max_rate,
		static_cast<int32_t>(pv_game->pv_data.targets.size())
	);

	if (state.chance_time.IsValid())
	{
		// NOTE: Add chance time star success bonus
		if (state.chance_time.successful)
			percentage += ChanceTimeRetainedRate;

		// NOTE: Add chance time bonus score percentage
		percentage += state.score.ct_score_bonus / static_cast<float>(state.score.max_ct_score_bonus) * ChanceTimePercBonus;
	}

	// NOTE: Add double tap bonus score percentage
	if (state.score.max_double_tap_bonus > 0)
		percentage += state.score.double_tap_bonus / static_cast<float>(state.score.max_double_tap_bonus) * DoubleTapPercBonus;

	// NOTE: Add sustain hold bonus score percentage
	if (state.score.max_sustain_bonus > 0)
		percentage += state.score.sustain_bonus / static_cast<float>(state.score.max_sustain_bonus) * SustainHoldPercBonus;

	return percentage * 100.0;
}

void score::CalculateScoreReference(ScoreState* ref, PVGameData* pv_game)
{
	const ChanceState& ct = state.chance_time;

	ref->target_max_rate = 1.0f;
	ref->max_ct_score_bonus = 0;
	ref->max_double_tap_bonus = 0;
	ref->max_sustain_bonus = 0;
	ref->max_link_bonus = 0;

	if (ct.IsValid())
		ref->target_max_rate -= ChanceTimeRetainedRate;

	const float target_step_score = (pv_game->reference_score * ref->target_max_rate) / pv_game->pv_data.targets.size();
	pv_game->target_reference_scores.clear();
	pv_game->target_reference_scores.push_back(0);

	for (size_t i = 0; i < pv_game->pv_data.targets.size(); i++)
	{
		const PvDscTargetGroup& group = pv_game->pv_data.targets[i];
		
		// NOTE: Calculate constant reference scores for percentage calculation
		if (ct.CheckTargetInRange(i))
			ref->max_ct_score_bonus += ChanceTimeScoreBonus[0];

		for (int32_t sub = 0; sub < group.target_count; sub++)
		{
			TargetStateEx& target = *GetTargetStateEx(i, sub);
			if (target.IsNormalDoubleNote())
				ref->max_double_tap_bonus += DoubleTapScoreBonus;

			if (target.IsLongNoteStart())
				ref->max_sustain_bonus += static_cast<int32_t>(target.length / SustainBonusInterval) * SustainBonusScore[0];

			if (target.IsLinkNote())
				ref->max_link_bonus += LinkNoteScoreBonus[0];
		}

		// NOTE: Patch target reference scores, to make the percentages borders
		//       move more accurately to the console gameplay flow.
		pv_game->target_reference_scores.push_back(pv_game->target_reference_scores.back() + target_step_score);
		if (ct.IsValid() && i == ct.last_target_index)
			pv_game->target_reference_scores.back() += pv_game->reference_score * ChanceTimeRetainedRate;
	}

	printf("Reference Score: %.2f %d %d %d\n", ref->target_max_rate, ref->max_double_tap_bonus, ref->max_sustain_bonus, ref->max_ct_score_bonus);
}