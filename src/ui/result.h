#pragma once

struct ScoreDetail
{
	uint8_t gap0[368];
	int32_t judge_count[5];
	int32_t judge_perc[5];
};

namespace nc
{
	inline bool ShouldUseConsoleStyleWin() { return state.GetScoreMode() != ScoreMode_Arcade; }

	void InitResultsData(ScoreDetail* detail);
	void DrawResultsWindowText(int32_t win);
	void InstallResultsHook();
}
