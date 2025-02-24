#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <Helpers.h>
#include <common.h>
#include "note_link.h"
#include "target.h"

static void PatchCommonKisekiColor(PvGameTarget* target);
static void UpdateLongNoteKiseki(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex, float dt);
static void DrawLongNoteKiseki(TargetStateEx* ex);

static const NoteSprite nc_target_layers[] = {
	{ nullptr,                nullptr, nullptr }, // 0x19 - Triangle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1A - Circle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1B - Cross Rush
	{ nullptr,                nullptr, nullptr }, // 0x1C - Square Rush
	{ "target_up_w",          nullptr, nullptr }, // 0x1D - Triangle W
	{ "target_right_w",       nullptr, nullptr }, // 0x1E - Circle W
	{ "target_down_w",        nullptr, nullptr }, // 0x1F - Cross W
	{ "target_left_w",        nullptr, nullptr }, // 0x20 - Square W
	{ "target_sankaku_long",  nullptr, nullptr }, // 0x21 - Triangle Long
	{ "target_maru_long",     nullptr, nullptr }, // 0x22 - Circle Long
	{ "target_batsu_long",    nullptr, nullptr }, // 0x23 - Cross Long
	{ "target_shikaku_long",  nullptr, nullptr }, // 0x24 - Square Long
	{ "target_touch",         nullptr, nullptr }, // 0x25 - Star
	{ nullptr,                nullptr, nullptr }, // 0x26 - Star Long
	{ "target_touch_w",       nullptr, nullptr }, // 0x27 - Star W
	{ "target_touch_ch_miss", nullptr, nullptr }, // 0x28 - Chance Star
	{ "target_link",          nullptr, nullptr }, // 0x29 - Link Star
	{ "target_link",          nullptr, nullptr }, // 0x2A - Link Star End
	{ nullptr,                nullptr, nullptr }, // 0x2B - Star Rush
};

static const NoteSprite nc_button_layers[] = {
	{ nullptr,                nullptr, nullptr }, // 0x19 - Triangle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1A - Circle Rush
	{ nullptr,                nullptr, nullptr }, // 0x1B - Cross Rush
	{ nullptr,                nullptr, nullptr }, // 0x1C - Square Rush
	{ "button_up_w",          nullptr, nullptr }, // 0x1D - Triangle W
	{ "button_right_w",       nullptr, nullptr }, // 0x1E - Circle W
	{ "button_down_w",        nullptr, nullptr }, // 0x1F - Cross W
	{ "button_left_w",        nullptr, nullptr }, // 0x20 - Square W
	{ "button_sankaku_long",  nullptr, nullptr }, // 0x21 - Triangle Long
	{ "button_maru_long",     nullptr, nullptr }, // 0x22 - Circle Long
	{ "button_batsu_long",    nullptr, nullptr }, // 0x23 - Cross Long
	{ "button_shikaku_long",  nullptr, nullptr }, // 0x24 - Square Long
	{ "button_touch",         nullptr, nullptr }, // 0x25 - Star
	{ nullptr,                nullptr, nullptr }, // 0x26 - Star Long
	{ "button_touch_w",       nullptr, nullptr }, // 0x27 - Star W
	{ "button_touch_ch_miss", nullptr, nullptr }, // 0x28 - Chance Star
	{ "button_link",          nullptr, nullptr }, // 0x29 - Link Star
	{ "button_link",          nullptr, nullptr }, // 0x2A - Link Star End
	{ nullptr,                nullptr, nullptr }, // 0x2B - Star Rush
};

static const char* GetProperLayerName(const NoteSprite* spr)
{
	// TODO: Add style logic
	if (spr->ft != nullptr)
		return spr->ft;

	return "target_sankaku";
}

const char* GetTargetLayer(int32_t target_type)
{
	if (target_type >= TargetType_Custom && target_type < TargetType_Max)
		return GetProperLayerName(&nc_target_layers[target_type - TargetType_Custom]);
	return nullptr;
}

const char* GetButtonLayer(int32_t target_type)
{
	if (target_type >= TargetType_Custom && target_type < TargetType_Max)
		return GetProperLayerName(&nc_button_layers[target_type - TargetType_Custom]);
	return nullptr;
}

int32_t PlayUpdateSoundEffect(PvGameTarget* target, TargetStateEx* ex, SoundEffect* se)
{
	switch (ex->target_type)
	{
	case TargetType_TriangleLong:
	case TargetType_CircleLong:
	case TargetType_CrossLong:
	case TargetType_SquareLong:
		if (ex != nullptr)
		{
			if (ex->se_state == SEState_Idle)
			{
				diva::sound::PlaySoundEffect(3, "800_button_l1_on", 1.0f);
				ex->se_queue = 3;
				ex->se_name = "800_button_l1_on";
				ex->se_state = SEState_Looping;
			}
			else if (ex->se_state == SEState_FailRelease)
			{
				diva::sound::ReleaseCue(ex->se_queue, ex->se_name.c_str(), false);
				ex->ResetSE();
			}
			else if (ex->se_state == SEState_SuccessRelease)
			{
				int32_t queue = ex->se_queue;
				diva::sound::ReleaseCue(ex->se_queue, ex->se_name.c_str(), true);
				ex->ResetSE();
				diva::sound::PlaySoundEffect(queue, "800_button_l1_off", 1.0f);
			}
		}

		return 1;
	case TargetType_UpW:
	case TargetType_RightW:
	case TargetType_DownW:
	case TargetType_LeftW:
		diva::sound::PlaySoundEffect(3, "400_button_w1", 1.0f);
		return 1;
	case TargetType_Star:
	case TargetType_LinkStar:
	case TargetType_LinkStarEnd:
	case TargetType_StarRush:
		diva::sound::PlaySoundEffect(3, "1200_scratch1", 1.0f);
		return 1;
	case TargetType_ChanceStar:
		if (ex != nullptr && ex->success)
		{
			diva::sound::PlaySoundEffect(3, "1516_cymbal", 1.0f);
			return 1;
		}

		diva::sound::PlaySoundEffect(3, "1200_scratch1", 1.0f);
		return 1;
	case TargetType_StarW:
		diva::sound::PlaySoundEffect(3, "1400_scratch_w1", 1.0f);
		return 1;
	}

	return 0;
};

HOOK(void, __fastcall, CreateTargetAetLayers, 0x14026F910, PvGameTarget* target)
{
	if (target->target_type < TargetType_Custom || target->target_type >= TargetType_Max)
		return originalCreateTargetAetLayers(target);

	TargetStateEx* ex = GetTargetStateEx(target->target_index, 0);
	ex->org = target;
	ex->target_pos = target->target_pos;

	// NOTE: Remove previously created aet objects, if present
	diva::aet::Stop(&target->target_aet);
	diva::aet::Stop(&target->button_aet);
	diva::aet::Stop(&target->target_eff_aet);
	diva::aet::Stop(&target->dword78);

	const char* target_layer = GetTargetLayer(target->target_type);
	const char* button_layer = GetButtonLayer(target->target_type);

	diva::vec2 target_pos;
	diva::vec2 button_pos;
	diva::GetScaledPosition(&target->target_pos, &target_pos);
	diva::GetScaledPosition(&target->button_pos, &button_pos);

	target->target_aet = diva::aet::PlayLayer(
		AetSceneID,
		8,
		0x20000,
		target_layer,
		&target_pos,
		0,
		nullptr,
		nullptr,
		0.0f,
		-1.0f,
		0,
		nullptr
	);

	target->button_aet = diva::aet::PlayLayer(
		AetSceneID,
		9,
		0x20000,
		button_layer,
		&button_pos,
		0,
		nullptr,
		nullptr,
		0.0f,
		-1.0f,
		0,
		nullptr
	);

	if (ex->link_step)
	{
		if (!ex->link_start)
			diva::aet::Stop(&target->button_aet);

		ex->target_aet = target->target_aet;
		ex->button_aet = target->button_aet;
		target->target_aet = 0;
		target->button_aet = 0;
	}

	// NOTE: Initialize some information
	target->out_start_time = 360.0f;
	target->scaling_end_time = 31.0f;
}

HOOK(void, __fastcall, UpdateTargets, 0x14026DD80, PVGameArcade* data, float dt)
{
	for (PvGameTarget* target = data->target; target != nullptr; target = target->next)
	{
		// NOTE: Update spawned link stars
		//
		TargetStateEx* ex = GetTargetStateEx(target->target_index);
		if (ex->link_start)
			state.PushTarget(ex);

		if (ex->link_step)
		{
			if (ex->flying_time_max <= 0.0f)
			{
				ex->flying_time_max = target->flying_time;
				ex->flying_time_remaining = target->flying_time_remaining;
			}
		}

		// NOTE: Update chance stars
		if (target->target_type == TargetType_ChanceStar)
		{
			if (state.chance_time.GetFillRate() == 15 && !ex->success)
			{
				diva::vec2 target_pos;
				diva::GetScaledPosition(&target->target_pos, &target_pos);

				diva::aet::Stop(&target->target_aet);
				target->target_aet = diva::aet::PlayLayer(
					AetSceneID,
					8,
					0x20000,
					"target_touch_ch",
					&target_pos,
					0,
					nullptr,
					nullptr,
					-1.0f,
					-1.0f,
					0,
					nullptr
				);

				diva::aet::Stop(&target->button_aet);
				target->button_aet = diva::aet::PlayLayer(
					AetSceneID,
					9,
					0x20000,
					"button_touch_ch",
					&target->button_pos,
					0,
					nullptr,
					nullptr,
					-1.0f,
					-1.0f,
					0,
					nullptr
				);

				ex->success = true;
			}
		}
	}
	
	if (ShouldUpdateTargets())
	{
		for (TargetStateEx* tgt : state.target_references)
		{
			if (tgt->link_start)
			{
				UpdateLinkStar(data, tgt, dt);
				UpdateLinkStarKiseki(data, tgt, dt);
			}
			else if (IsLongNote(tgt->target_type) && tgt->holding)
				UpdateLongNoteKiseki(data, nullptr, tgt, dt);
		}
	}

	return originalUpdateTargets(data, dt);
}

HOOK(void, __fastcall, PVGameReset, 0x1402436F0, void* pv_game)
{
	printf("[NC] Reset!\n");
	
	state.ResetPlayState();
	originalPVGameReset(pv_game);
}

HOOK(void, __fastcall, UpdateKiseki, 0x14026F050, PVGameArcade* data, PvGameTarget* target, float dt)
{
	if (target->target_type < TargetType_Custom)
		return originalUpdateKiseki(data, target, dt);

	if (IsLongNote(target->target_type))
	{
		if (TargetStateEx* ex = GetTargetStateEx(target->target_index); ex != nullptr)
		{
			ex->kiseki_pos = target->button_pos;
			ex->kiseki_dir = target->delta_pos_sq;
			UpdateLongNoteKiseki(data, target, ex, dt);
		}
	}
	else
	{
		originalUpdateKiseki(data, target, dt);
		PatchCommonKisekiColor(target);
	}
}

HOOK(void, __fastcall, DrawKiseki, 0x140271030, PvGameTarget* target)
{
	TargetStateEx* ex = GetTargetStateEx(target->target_index);
	if (IsLongNote(target->target_type))
	{
		DrawLongNoteKiseki(ex);
		return;
	}
	else if (IsLinkStarNote(target->target_type, false) && !ex->link_start)
		return;

	originalDrawKiseki(target);
};

HOOK(void, __fastcall, DrawArcadeGame, 0x140271AB0, PVGameArcade* data)
{
	for (TargetStateEx* tgt : state.target_references)
	{
		if (IsLongNote(tgt->target_type) && tgt->holding)
			DrawLongNoteKiseki(tgt);
		else if (tgt->link_start)
		{
			for (TargetStateEx* ex = tgt; ex != nullptr; ex = ex->next)
			{
				if (ex->vertex_count_max != 0)
				{
					if (!tgt->IsChainSucessful())
						DrawTriangles(ex->kiseki.data(), ex->vertex_count_max, 13, 7, 752437696);
					else
						DrawTriangles(ex->kiseki.data(), ex->vertex_count_max, 13, 7, 716223682);
				}
			}
		}
	}

	originalDrawArcadeGame(data);
}

static void PatchCommonKisekiColor(PvGameTarget* target)
{
	float r, g, b;
	TargetStateEx* ex = GetTargetStateEx(target->target_index);

	switch (target->target_type)
	{
	case TargetType_TriangleRush:
	case TargetType_UpW:
		r = 0.799f;
		g = 1.0f;
		b = 0.5401;
		break;
	case TargetType_CircleRush:
	case TargetType_RightW:
		r = 0.9372f;
		g = 0.2705f;
		b = 0.2901f;
		break;
	case TargetType_CrossRush:
	case TargetType_DownW:
		r = 0.7098f;
		g = 1.0f;
		b = 1.0f;
		break;
	case TargetType_SquareRush:
	case TargetType_LeftW:
		r = 1.0f;
		g = 0.8117f;
		b = 1.0f;
		break;
	case TargetType_Star:
	case TargetType_StarW:
	case TargetType_LinkStar:
	case TargetType_LinkStarEnd:
	case TargetType_StarRush:
		r = 0.9f;
		g = 0.9f;
		b = 0.1f;
		break;
	case TargetType_ChanceStar:
		if (ex != nullptr && ex->success)
		{
			r = 0.9f;
			g = 0.9f;
			b = 0.1f;
		}
		else
		{
			r = 0.65f;
			g = 0.65f;
			b = 0.65f;
		}

		break;
	default:
		r = 0.0f;
		g = 0.0f;
		b = 0.0f;
		break;
	}

	uint32_t color = (uint8_t)(r * 255) |
		((uint8_t)(g * 255) << 8) |
		((uint8_t)(b * 255) << 16);

	for (int i = 0; i < 40; i++)
		target->kiseki[i].color = (target->kiseki[i].color & 0xFF000000) | (color & 0x00FFFFFF);
}

static void UpdateLongNoteKiseki(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex, float dt)
{
	const float width = 12.0f;

	// NOTE: Initialize vertex buffer
	if (ex->kiseki.size() < 1 && target != nullptr)
	{
		ex->vertex_count_max = ex->length * KisekiRate * 2;
		if (ex->vertex_count_max % 2 != 0)
			ex->vertex_count_max += 3;

		ex->kiseki.resize(ex->vertex_count_max);

		for (size_t i = 0; i < ex->vertex_count_max; i += 2)
		{
			ex->kiseki[i].pos = target->kiseki[0].pos;
			ex->kiseki[i].uv = target->kiseki[0].uv;
			ex->kiseki[i].color = 0xFFFFFFFF;
			ex->kiseki[i + 1].pos = target->kiseki[1].pos;
			ex->kiseki[i + 1].uv = target->kiseki[1].uv;
			ex->kiseki[i + 1].color = 0xFFFFFFFF;
		}
	}

	// NOTE: Update vertices
	ex->kiseki_time += dt;
	size_t count = 0;
	while (ex->kiseki_time >= 1.0f / KisekiRate)
	{
		// NOTE: Move vertices back
		if (ex->vertex_count_max >= 4)
		{
			for (int32_t i = static_cast<int32_t>(ex->vertex_count_max) - 4; i >= 0; i -= 2)
			{
				ex->kiseki[i + 2].pos = ex->kiseki[i].pos;
				ex->kiseki[i + 3].pos = ex->kiseki[i + 1].pos;
			}

			count++;
		}

		ex->kiseki_time -= 1.0f / KisekiRate;
	}

	// NOTE: Update position
	for (size_t i = 0; i < count; i++)
	{
		float offset_x = ex->kiseki_dir.x * (width / 2.0f);
		float offset_y = ex->kiseki_dir.y * (width / 2.0f);

		diva::vec3 left = {
			ex->kiseki_pos.x - (ex->kiseki_dir.x * width) + offset_x,
			ex->kiseki_pos.y - (ex->kiseki_dir.y * width) + offset_y,
			0.0f
		};

		diva::vec3 right = {
			ex->kiseki_pos.x + (ex->kiseki_dir.x * width) + offset_x,
			ex->kiseki_pos.y + (ex->kiseki_dir.y * width) + offset_y,
			0.0f
		};

		diva::GetScaledPosition((diva::vec2*)&left, (diva::vec2*)&left);
		diva::GetScaledPosition((diva::vec2*)&right, (diva::vec2*)&right);

		ex->kiseki[i * 2].pos = right;
		ex->kiseki[i * 2].uv.y = 0.5f;
		ex->kiseki[i * 2].color = 0xFFFFFFFF;
		ex->kiseki[i * 2 + 1].pos = left;
		ex->kiseki[i * 2 + 1].uv.y = 1.0f;
		ex->kiseki[i * 2 + 1].color = 0xFFFFFFFF;
	}

	// NOTE: Update UV
	for (size_t i = 0; i < ex->vertex_count_max; i++)
		ex->kiseki[i].uv.x += dt * 128.0f;
}

static void DrawLongNoteKiseki(TargetStateEx* ex)
{
	if (ex->vertex_count_max != 0)
	{
		int32_t sprite_id = 0x3AB;
		switch (ex->target_type)
		{
		case TargetType_TriangleLong:
			sprite_id = 2547777446; // SPR_GAM_CMN_KISEKI02
			break;
		case TargetType_CircleLong:
			sprite_id = 268038873;  // SPR_GAM_CMN_KISEKI03
			break;
		case TargetType_CrossLong:
			sprite_id = 3751393426; // SPR_GAM_CMN_KISEKI04
			break;
		case TargetType_SquareLong:
			sprite_id = 1382161087; // SPR_GAM_CMN_KISEKI05
			break;
		case TargetType_StarLong:
			sprite_id = 337203558;  // SPR_GAM_CMN_KISEKI06
			break;
		}

		DrawTriangles(ex->kiseki.data(), ex->vertex_count_max, 13, 7, sprite_id);
	}
}

void InstallTargetHooks()
{
	INSTALL_HOOK(CreateTargetAetLayers);
	INSTALL_HOOK(UpdateTargets);
	INSTALL_HOOK(UpdateKiseki);
	INSTALL_HOOK(DrawKiseki);
	INSTALL_HOOK(DrawArcadeGame);
	INSTALL_HOOK(PVGameReset);
}