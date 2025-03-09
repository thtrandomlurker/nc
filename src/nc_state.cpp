#include "nc_state.h"

void TargetStateEx::ResetPlayState()
{
	hold_button = nullptr;
	org = nullptr;
	force_hit_state = HitState_None;
	hit_state = HitState_None;
	flying_time_max = 0.0f;
	flying_time_remaining = 0.0f;
	delta_time_max = 0.0f;
	delta_time = 0.0f;
	length_remaining = length;
	kiseki_time = 0.0f;
	alpha = 0.0f;
	holding = false;
	success = false;
	current_step = false;
	step_state = LinkStepState_None;
	link_ending = false;
	aet::Stop(&target_aet);
	aet::Stop(&button_aet);
	kiseki_pos = { 0.0f, 0.0f };
	kiseki_dir = { 0.0f, 0.0f };
	kiseki_dir_norot = { 0.0f, 0.0f };
	kiseki.clear();
	vertex_count_max = 0;
	long_bonus_timer = 0.0f;
	score_bonus = 0;
	ct_score_bonus = 0;
	double_tapped = false;
}

bool TargetStateEx::IsChainSucessful()
{
	bool cond = true;
	for (TargetStateEx* ex = this; ex != nullptr; ex = ex->next)
		cond = cond && (ex->hit_state >= HitState_Cool && ex->hit_state <= HitState_Sad);

	return cond;
}

void TargetStateEx::StopAet(bool button, bool target, bool kiseki)
{
	if (button)
		aet::Stop(&button_aet);
	if (target)
		aet::Stop(&target_aet);
	if (kiseki)
	{
		this->kiseki.clear();
		vertex_count_max = 0;
	}
}

bool TargetStateEx::SetLongNoteAet()
{
	if (org == nullptr)
		return false;

	// NOTE: Copy target aet and set play params
	target_aet = org->target_aet;
	aet::SetPlay(target_aet, false);
	aet::SetFrame(target_aet, 360.0f);

	// NOTE: Free original aets
	org->target_aet = 0;
	aet::Stop(&org->button_aet);
	aet::Stop(&org->target_eff_aet);
	aet::Stop(&org->dword78);

	// TODO: Set scale to 1.0;
	//       Fix KISEKI pos when hit after the cool window!!!
	//

	return true;
}

bool TargetStateEx::SetLinkNoteAet()
{
	if (org == nullptr || !IsLinkNoteStart())
		return false;

	target_aet = org->target_aet;
	button_aet = org->button_aet;
	org->target_aet = 0;
	org->button_aet = 0;
	aet::Stop(&org->target_eff_aet);
	aet::Stop(&org->dword78);

	return true;
}

void UIState::SetLayer(int32_t index, bool visible, const char* name, int32_t prio, int32_t flags)
{
	aet::Stop(&aet_list[index]);
	aet_visibility[index] = visible;
	aet_list[index] = aet::PlayLayer(
		AetSceneID,
		prio,
		flags,
		name,
		nullptr,
		0,
		nullptr,
		nullptr,
		-1.0f,
		-1.0f,
		0,
		nullptr
	);
}

void UIState::ResetAllLayers()
{
	for (int i = 0; i < LayerUI_Max; i++)
	{
		aet::Stop(&aet_list[i]);
		aet_visibility[i] = false;
	}
}

void StateEx::ResetPlayState()
{
	target_references.clear();
	for (TargetStateEx& ex : target_ex)
		ex.ResetPlayState();
	chance_time.ResetPlayState();
}

void StateEx::Reset()
{
	target_references.clear();
	target_ex.clear();
	chance_time.first_target_index = -1;
	chance_time.last_target_index = -1;
	chance_time.targets_hit = 0;
}

bool StateEx::PushTarget(TargetStateEx* ex)
{
	for (TargetStateEx* p : target_references)
		if (p == ex)
			return false;

	target_references.push_back(ex);
	return true;
}

bool StateEx::PopTarget(TargetStateEx* ex)
{
	for (auto it = target_references.begin(); it != target_references.end(); it++)
	{
		if (*it == ex)
		{
			target_references.erase(it);
			return true;
		}
	}

	return false;
}

void StateEx::PlaySoundEffect(int32_t type)
{
	switch (type)
	{
	case SEType_Double:
		sound::PlaySoundEffect(3, "400_button_w1", 1.0f);
		break;
	case SEType_LongStart:
		sound::ReleaseCue(3, "800_button_l1_on", true);
		sound::PlaySoundEffect(3, "800_button_l1_on", 1.0f);
		break;
	case SEType_LongRelease:
		sound::ReleaseCue(3, "800_button_l1_on", true);
		sound::PlaySoundEffect(3, "800_button_l1_off", 1.0f);
		break;
	case SEType_LongFail:
		sound::ReleaseCue(3, "800_button_l1_on", false);
		break;
	case SEType_Star:
		sound::PlaySoundEffect(3, "1200_scratch1", 1.0f);
		break;
	case SEType_Cymbal:
		sound::PlaySoundEffect(3, "1516_cymbal", 1.0f);
		break;
	case SEType_StarDouble:
		sound::PlaySoundEffect(3, "1400_scratch_w1", 1.0f);
		break;
	}
}

TargetStateEx* GetTargetStateEx(int32_t index, int32_t sub_index)
{
	for (TargetStateEx& ex : state.target_ex)
		if (ex.target_index == index && ex.sub_index == sub_index)
			return &ex;
	return nullptr;
}

TargetStateEx* GetTargetStateEx(const PvGameTarget* org)
{
	int32_t sub_index = 0;
	for (PvGameTarget* prev = org->prev; prev != nullptr; prev = prev->prev)
	{
		if (prev->multi_count != org->multi_count || org->multi_count == -1)
			break;

		sub_index++;
	}

	return GetTargetStateEx(org->target_index, sub_index);
}