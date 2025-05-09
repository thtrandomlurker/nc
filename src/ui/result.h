#pragma once

struct ScoreDetail
{
	uint8_t gap0[368];
	int32_t judge_count[5];
	int32_t judge_perc[5];
};

namespace results
{
	constexpr uint32_t AetSetID   = 14010070;
	constexpr uint32_t SprSetID   = 14020070;
	constexpr uint32_t AetSceneID = 14010071;
}

namespace nc
{
	bool ShouldUseConsoleStyleWin();

	void InitResultsData(ScoreDetail* detail);
	void DrawResultsWindowText(int32_t win);
	void InstallResultsHook();
}
