#include <nc_log.h>
#include "hit_state.h"
#include "tech_zone.h"

bool TechZoneState::IsValid() const
{
	return first_target_index != -1 && last_target_index != -1;
}

int32_t TechZoneState::GetTargetCount() const
{
	if (!IsValid())
		return 0;
	return last_target_index - first_target_index + 1;
}

int32_t TechZoneState::GetRemainingCount() const
{
	if (!IsValid())
		return 0;
	return GetTargetCount() - targets_hit;
}

bool TechZoneState::PushNewHitState(int32_t target_index, int32_t hit_state)
{
	if (!IsValid())
		return false;

	if (target_index >= first_target_index && target_index <= last_target_index)
	{
		if (nc::IsHitGreat(hit_state) && !failed)
			targets_hit++;
		else
			failed = true;

		nc::Print("%d  CONTINUE:%s  REMAINING:%d\n", hit_state, failed ? "NO" : "YES", GetRemainingCount());
	}
}

void TechZoneState::ResetPlayState()
{
	targets_hit = 0;
	failed = false;
}