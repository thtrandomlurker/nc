#pragma once

#include <nc_state.h>

namespace nc
{
	bool CheckHit(int32_t hit_state, bool wrong, bool worst);
	int32_t JudgeNoteHit(PVGameArcade* game, PvGameTarget** group, TargetStateEx** extras, int32_t group_count, bool* success);
	bool CheckLongNoteHolding(TargetStateEx* ex);
	bool CheckRushNotePops(TargetStateEx* ex);
}