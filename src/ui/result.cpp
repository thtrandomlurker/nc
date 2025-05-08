#include <stdint.h>
#include <hooks.h>
#include <util.h>
#include <nc_state.h>
#include "common.h"
#include "result.h"

static int32_t total_bonus_score = 0;
static std::pair<int32_t, int32_t> ct_perc;
static uint32_t ct_result_txt_id = 0;

static void DrawNumberOrText(const AetComposition& comp, const char* layer, const char* format, int32_t number = 0)
{
	if (auto it = comp.find(layer); it != comp.end())
	{
		const AetLayout& layout = it->second;

		// NOTE: Set up font
		FontInfo font = FontInfo::CreateSpriteFont(49485, 34, 32);
		font.SetSize(font.size.x * layout.matrix.row0.x, font.size.y * layout.matrix.row1.y);

		// NOTE: Draw text
		TextArgs args = { };
		args.print_work.text_current = GetLayoutAdjustedPosition(layout, layer);
		args.print_work.line_origin = args.print_work.text_current;
		args.print_work.font = &font;
		args.print_work.res_mode = 14;
		args.print_work.prio = 6;
		args.print_work.SetColor(0xFFFFFFFF, 0xFFFFFFFF);
		args.print_work.SetOpacity(layout.opacity);

		spr::DrawTextA(&args, 0x2, util::Format(format, number).c_str());
	}
}

static int32_t CalculateMaxRatePercentage(ScoreDetail* detail, int32_t index, float target_max_rate)
{
	float all_count = detail->judge_count[0] +
		detail->judge_count[1] +
		detail->judge_count[2] +
		detail->judge_count[3] +
		detail->judge_count[4];

	float perc = detail->judge_count[index] / all_count * target_max_rate * 100.0f;
	return static_cast<int32_t>(perc * 100);
}

void nc::InitResultsData(ScoreDetail* detail)
{
	total_bonus_score = state.CalculateTotalBonusScore();
	ct_perc = { -1, -1 };
	ct_result_txt_id = 0;

	if (state.chance_time.IsValid())
	{
		ct_perc = {
			ChanceTimeRetainedRate * 100,
			static_cast<int32_t>(ChanceTimeRetainedRate * 100000) % 100
		};

		ct_result_txt_id = state.chance_time.successful ? 3031571741 : 2360194960;
	}

	if (detail)
	{
		if (state.GetScoreMode() == ScoreMode_F2nd)
		{
			detail->judge_perc[0] = CalculateMaxRatePercentage(detail, 0, state.score.target_max_rate);
			detail->judge_perc[1] = CalculateMaxRatePercentage(detail, 1, state.score.target_max_rate);
			detail->judge_perc[2] = 0;
			detail->judge_perc[3] = 0;
			detail->judge_perc[4] = 0;
		}
		else if (state.GetScoreMode() == ScoreMode_Franken)
		{
			for (int32_t i = 0; i < 5; i++)
				detail->judge_perc[i] = CalculateMaxRatePercentage(detail, i, state.score.target_max_rate);
		}
	}
}

void nc::DrawResultsWindowText(int32_t win)
{
	AetComposition comp;
	aet::GetComposition(&comp, win);

	DrawNumberOrText(comp, "p_ct_per_01_rt", ct_perc.first > -1 ? "%d" : "-", ct_perc.first);
	DrawNumberOrText(comp, "p_ct_per_02_rt", ct_perc.second > -1 ? "%02d" : "--", ct_perc.second);
	if (ct_result_txt_id != 0)
	{
		if (auto it = comp.find("p_ct_result_txt"); it != comp.end())
			DrawSpriteAtLayout(it->second, "p_ct_result_txt", ct_result_txt_id, 6, 14);
	}

	DrawNumberOrText(comp, "p_tz_count_num_01_rt", "-");
	DrawNumberOrText(comp, "p_tz_count_num_02_rt", "-");
	DrawNumberOrText(comp, "p_tz_per_01_rt", "-");
	DrawNumberOrText(comp, "p_tz_per_02_rt", "--");
	DrawNumberOrText(comp, "p_bonus_num_rt", total_bonus_score <= 0 ? "%d" : "%+d", total_bonus_score);
}

void InstallResultPS4Hooks();

void nc::InstallResultsHook()
{
	if (game::IsFutureToneMode())
		InstallResultPS4Hooks();
}