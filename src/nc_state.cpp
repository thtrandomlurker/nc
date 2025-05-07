#include "save_data.h"
#include "sound_db.h"
#include "helpers.h"
#include "shared.h"
#include "nc_state.h"

void TargetStateEx::ResetPlayState()
{
	hold_button = nullptr;
	org = nullptr;
	force_hit_state = HitState_None;
	hit_state = HitState_None;
	hit_time = 0.0f;
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
	kiseki_pos = { 0.0f, 0.0f };
	kiseki_dir = { 0.0f, 0.0f };
	kiseki_dir_norot = { 0.0f, 0.0f };
	kiseki.clear();
	vertex_count_max = 0;
	fix_long_kiseki = false;
	sustain_bonus_time = 0.0f;
	score_bonus = 0;
	ct_score_bonus = 0;
	double_tapped = false;
	bal_hit_count = 0;
	bal_scale = 0.0f;
	ResetAetData();
}

void TargetStateEx::ResetAetData()
{
	aet::Stop(&target_aet);
	aet::Stop(&button_aet);
	aet::Stop(&bal_effect_aet);
	kiseki.clear();
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
	aet::Stop(&bal_effect_aet);
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

	diva::vec3 scale = { 1.0f, 1.0f, 1.0f };
	aet::SetScale(target_aet, &scale);

	// NOTE: Free original aets
	org->target_aet = 0;
	aet::Stop(&org->button_aet);
	aet::Stop(&org->target_eff_aet);
	aet::Stop(&org->dword78);

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

bool TargetStateEx::SetRushNoteAet()
{
	if (org == nullptr || !IsRushNote())
		return false;

	button_aet = org->button_aet;
	aet::SetPlay(button_aet, false);
	aet::SetFrame(button_aet, bal_time);

	diva::vec2 scaled_pos = GetScaledPosition(target_pos);
	diva::vec3 pos = { scaled_pos.x, scaled_pos.y, 0.0f };
	aet::SetPosition(button_aet, &pos);

	diva::vec3 scale = { 1.0f, 1.0f, 1.0f };
	aet::SetScale(button_aet, &scale);

	org->button_aet = 0;
	aet::Stop(&org->target_eff_aet);
	aet::Stop(&org->dword78);

	if (bal_effect_aet != 0)
		aet::SetPlay(bal_effect_aet, true);
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

void SoundEffects::SetSoundEffects(const SoundEffect& org)
{
	auto tryFindSound = [&](int32_t id, const std::vector<SoundInfo>& data, std::string fallback = "", int32_t default_id = -1)
	{
		if (id > 0)
		{
			if (auto* info = FindWithID(data, id); info != nullptr)
				return info->se_name;
		}
		else if (id == -2 && default_id != -1 && state.nc_song_entry.has_value())
		{
			switch (default_id)
			{
			case 0:
				return state.nc_song_entry.value().star_se_name;
			default:
				break;
			}
		}

		return fallback;
	};

	const auto& config = *nc::GetConfigSet();
	w_button = tryFindSound(config.button_w_se_id, *sound_db::GetButtonWSoundDB(), org.button.c_str());
	l_button_on = tryFindSound(config.button_l_se_id, *sound_db::GetButtonLongOnSoundDB(), org.button.c_str());
	l_button_off = tryFindSound(config.button_l_se_id, *sound_db::GetButtonLongOffSoundDB(), org.button.c_str());
	star = tryFindSound(config.star_se_id, *sound_db::GetStarSoundDB(), "", 0);
	w_star = tryFindSound(config.star_w_se_id, *sound_db::GetStarWSoundDB(), star);
	link = tryFindSound(config.link_se_id, *sound_db::GetLinkSoundDB(), "");
	rush_on = "se_pv_button_rush1_on";
	rush_off = "se_pv_button_rush1_off";
}

void StateEx::ResetPlayState()
{
	target_references.clear();
	for (TargetStateEx& ex : target_ex)
		ex.ResetPlayState();
	chance_time.ResetPlayState();
	score.ct_score_bonus = 0;
	score.double_tap_bonus = 0;
	score.sustain_bonus = 0;
	score.link_bonus = 0;
	score.rush_bonus = 0;
	ResetAetData();
}

void StateEx::ResetAetData()
{
	for (int i = 0; i < MaxHitEffectCount; i++)
		aet::Stop(&effect_buffer[i]);
	effect_index = 0;
	ui.ResetAllLayers();
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
		sound::PlaySoundEffect(3, sound_effects.w_button.c_str(), 1.0f);
		break;
	case SEType_LongStart:
		sound::ReleaseCue(3, sound_effects.l_button_on.c_str(), true);
		sound::PlaySoundEffect(3, sound_effects.l_button_on.c_str(), 1.0f);
		break;
	case SEType_LongRelease:
		sound::ReleaseCue(3, sound_effects.l_button_on.c_str(), true);
		sound::PlaySoundEffect(3, sound_effects.l_button_off.c_str(), 1.0f);
		break;
	case SEType_LongFail:
		sound::ReleaseCue(3, sound_effects.l_button_on.c_str(), false);
		break;
	case SEType_Star:
		sound::PlaySoundEffect(3, sound_effects.star.c_str(), 1.0f);
		break;
	case SEType_Cymbal:
		sound::PlaySoundEffect(3, "cymbal_mmv", 1.0f);
		break;
	case SEType_StarDouble:
		sound::PlaySoundEffect(3, sound_effects.w_star.c_str(), 1.0f);
		break;
	case SEType_RushStart:
		sound::ReleaseCue(3, sound_effects.rush_on.c_str(), true);
		sound::PlaySoundEffect(3, sound_effects.rush_on.c_str(), 1.0f);
		break;
	case SEType_RushPop:
		sound::ReleaseCue(3, sound_effects.rush_on.c_str(), true);
		sound::PlaySoundEffect(3, sound_effects.rush_off.c_str(), 1.0f);
		break;
	case SEType_RushFail:
		sound::ReleaseCue(3, sound_effects.rush_on.c_str(), false);
		break;
	case SEType_LinkStart:
		if (!sound_effects.link.empty())
			sound::PlaySoundEffect(3, sound_effects.link.c_str(), 1.0f);
		break;
	case SEType_LinkEnd:
		if (!sound_effects.link.empty())
			sound::ReleaseCue(3, sound_effects.link.c_str(), false);
		break;
	}
}

void StateEx::PlayRushHitEffect(const diva::vec2& pos, float scale, bool pop)
{
	if (effect_index >= MaxHitEffectCount)
		effect_index = 0;

	uint32_t scene_id = pop ? AetSceneID : 3;
	const char* layer_name = pop ? "bal_hit_eff" : "hit_eff00";
	int32_t max_keep = pop ? 4 : 2;

	aet::Stop(&effect_buffer[effect_index]);
	effect_buffer[effect_index] = aet::PlayLayer(
		scene_id,
		8,
		0x20000,
		layer_name,
		&pos,
		0,
		nullptr,
		nullptr,
		-1.0f,
		-1.0f,
		0,
		nullptr
	);

	diva::vec3 scl = { scale, scale, 1.0f };
	aet::SetScale(effect_buffer[effect_index], &scl);

	int32_t count = 0;
	int32_t start = effect_index > 0 ? effect_index - 1 : MaxHitEffectCount - 1;
	for (int i = start; i > 0; i--)
	{
		// NOTE: Break early if we're back to the initial point. Should be when we
		//       loop back around to it.
		if (i == effect_index)
			break;

		// NOTE: Remove hit effect aet if it exceeds the max count
		if (count + 1 > max_keep)
			aet::Stop(&effect_buffer[i]);

		count++;

		// NOTE: Loop around to the back of the array
		if (i == 0)
			i = MaxHitEffectCount - 1;
	}

	effect_index++;
}

int32_t StateEx::GetScoreMode() const
{
	if (GetGameStyle() == GameStyle_Console)
		return ScoreMode_F2nd;
	return ScoreMode_Arcade;
}

int32_t StateEx::GetGameStyle() const
{
	if (nc_chart_entry.has_value())
		return nc_chart_entry.value().style;
	return GameStyle_Arcade;
}

int32_t StateEx::CalculateTotalBonusScore() const
{
	return score.ct_score_bonus + score.double_tap_bonus + score.sustain_bonus + score.link_bonus + score.rush_bonus;
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