#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <hooks.h>
#include "note_link.h"
#include "target.h"

static void PatchCommonKiseki(PvGameTarget* target);
static void UpdateLongNoteKiseki(PVGameArcade* data, TargetStateEx* ex, float dt);
static void DrawLongNoteKiseki(TargetStateEx* ex);
static void DrawBalloonEffect(TargetStateEx* ex);

static const NoteSprite nc_target_layers[] = {
	{ "target_sankaku_bal",   nullptr, nullptr }, // 0x19 - Triangle Rush
	{ "target_maru_bal",      nullptr, nullptr }, // 0x1A - Circle Rush
	{ "target_batsu_bal",     nullptr, nullptr }, // 0x1B - Cross Rush
	{ "target_shikaku_bal",   nullptr, nullptr }, // 0x1C - Square Rush
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
	{ "target_touch_bal",     nullptr, nullptr }, // 0x2B - Star Rush
};

static const NoteSprite nc_button_layers[] = {
	{ "button_sankaku_bal",   nullptr, nullptr }, // 0x19 - Triangle Rush
	{ "button_maru_bal",      nullptr, nullptr }, // 0x1A - Circle Rush
	{ "button_batsu_bal",     nullptr, nullptr }, // 0x1B - Cross Rush
	{ "button_shikaku_bal",   nullptr, nullptr }, // 0x1C - Square Rush
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
	{ "button_touch_bal",     nullptr, nullptr }, // 0x2B - Star Rush
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

HOOK(void, __fastcall, CreateTargetAetLayers, 0x14026F910, PvGameTarget* target)
{
	// NOTE: Copy some stuff we might want to keep handy
	TargetStateEx* ex = GetTargetStateEx(target);
	ex->org = target;
	ex->target_pos = target->target_pos;

	if (target->target_type < TargetType_Custom || target->target_type >= TargetType_Max)
		return originalCreateTargetAetLayers(target);

	// NOTE: Call the original routine with a temporary note type to have it calculate
	//       some other data (so that we don't have to duplicate all that code here)
	int32_t type = target->target_type;
	target->target_type = TargetType_Triangle;
	originalCreateTargetAetLayers(target);
	target->target_type = type;

	// NOTE: Remove previously created aet objects, if present
	aet::Stop(&target->target_aet);
	aet::Stop(&target->button_aet);
	aet::Stop(&target->target_eff_aet);
	aet::Stop(&target->dword78);

	const char* target_layer = GetTargetLayer(target->target_type);
	const char* button_layer = GetButtonLayer(target->target_type);

	diva::vec2 target_pos = GetScaledPosition(target->target_pos);
	diva::vec2 button_pos = GetScaledPosition(target->button_pos);

	target->target_aet = aet::PlayLayer(
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

	target->button_aet = aet::PlayLayer(
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
			aet::Stop(&target->button_aet);

		ex->target_aet = target->target_aet;
		ex->button_aet = target->button_aet;
		target->target_aet = 0;
		target->button_aet = 0;
	}

	if (ex->IsRushNote())
	{
		ex->bal_time = aet::GetMarkerTime(AetSceneID, button_layer, "bal");
		ex->bal_start_time = aet::GetMarkerTime(AetSceneID, button_layer, "bal_start");
		ex->bal_end_time = aet::GetMarkerTime(AetSceneID, button_layer, "bal_end");
		ex->bal_effect_aet = aet::PlayLayer(AetSceneID, 12, 0x10000, "target_balloon_eff", nullptr, 0, nullptr, nullptr, -1.0f, -1.0f, 0, nullptr);
		aet::SetPlay(ex->bal_effect_aet, false);
	}

	aet::SetOpacity(target->target_aet, target->target_opacity);
	aet::SetOpacity(target->button_aet, target->button_opacity);
}

HOOK(void, __fastcall, UpdateTargets, 0x14026DD80, PVGameArcade* data, float dt)
{
	for (PvGameTarget* target = data->target; target != nullptr; target = target->next)
	{
		// NOTE: Update spawned link stars
		//
		TargetStateEx* ex = GetTargetStateEx(target);
		if (ex->link_start || ex->IsLongNoteStart() || ex->IsRushNote())
			state.PushTarget(ex);

		if (ex->flying_time_max <= 0.0f)
		{
			ex->flying_time_max = target->flying_time;
			ex->flying_time_remaining = target->flying_time_remaining;
		}

		// NOTE: Update chance stars
		if (target->target_type == TargetType_ChanceStar)
		{
			if (state.chance_time.GetFillRate() == 15 && !ex->success)
			{
				diva::vec2 target_pos = GetScaledPosition(target->target_pos);

				aet::Stop(&target->target_aet);
				target->target_aet = aet::PlayLayer(
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

				aet::Stop(&target->button_aet);
				target->button_aet = aet::PlayLayer(
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
			if (tgt->IsLinkNoteStart())
			{
				UpdateLinkStar(data, tgt, dt);
				UpdateLinkStarKiseki(data, tgt, dt);
			}
			else if (tgt->IsLongNoteStart() && tgt->holding)
				UpdateLongNoteKiseki(data, tgt, dt);

			// NOTE: Update rush / long note length and timer
			if (tgt->IsLongNoteStart() && tgt->holding)
			{
				tgt->length_remaining = fmaxf(tgt->length_remaining - dt, 0.0f);
				tgt->long_bonus_timer += dt;
			}
			else if (tgt->IsRushNote() && tgt->holding)
			{
				if (tgt->length_remaining <= 0.0f)
				{
					tgt->holding = false;
					if (tgt->bal_hit_count >= tgt->bal_max_hit_count)
					{
						diva::vec2 pos = GetScaledPosition(tgt->target_pos);
						state.PlayRushHitEffect(pos, 1.0f + tgt->bal_scale, true);
						state.PlaySoundEffect(SEType_RushPop);
					}
					else
					{
						state.PlaySoundEffect(SEType_RushFail);
					}

					tgt->StopAet();
				}
				else if (tgt->length_remaining <= tgt->flying_time_max)
				{
					if (tgt->flying_time_max > 0.0f)
					{
						float progress = (tgt->flying_time_max - tgt->length_remaining) / tgt->flying_time_max;
						float frame = tgt->bal_start_time + (tgt->bal_end_time - tgt->bal_start_time) * progress;
						aet::SetFrame(tgt->button_aet, frame);
					}
				}

				// NOTE: Set rush note scale
				float scale = 1.0f + tgt->bal_scale;
				diva::vec3 scale_vec = { scale, scale, 1.0f };
				aet::SetScale(tgt->button_aet, &scale_vec);

				tgt->length_remaining = fmaxf(tgt->length_remaining - dt, 0.0f);
			}

			for (TargetStateEx* chain = tgt; chain != nullptr; chain = chain->next)
			{
				if (chain->flying_time_max >= 0.0f)
					chain->flying_time_remaining -= dt;
			}
		}
	}

	originalUpdateTargets(data, dt);
}

HOOK(void, __fastcall, UpdateKiseki, 0x14026F050, PVGameArcade* data, PvGameTarget* target, float dt)
{
	TargetStateEx* ex = GetTargetStateEx(target);
	if (ex->IsLongNote())
	{
		ex->kiseki_pos = target->button_pos;
		ex->kiseki_dir = target->delta_pos_sq;
		UpdateLongNoteKiseki(data, ex, dt);
	}
	else
	{
		originalUpdateKiseki(data, target, dt);
		PatchCommonKiseki(target);
	}
}

HOOK(void, __fastcall, DrawKiseki, 0x140271030, PvGameTarget* target)
{
	TargetStateEx* ex = GetTargetStateEx(target);
	if (ex->IsLongNote())
		return DrawLongNoteKiseki(ex);
	else if (ex->IsLinkNote() && !ex->IsLinkNoteStart())
		return;

	originalDrawKiseki(target);
};

HOOK(void, __fastcall, DrawArcadeGame, 0x140271AB0, PVGameArcade* data)
{
	for (TargetStateEx* tgt : state.target_references)
	{
		if (tgt->IsLongNoteStart() && tgt->holding)
			DrawLongNoteKiseki(tgt);
		else if (tgt->IsLinkNoteStart())
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
		else if (tgt->IsRushNote() && tgt->holding)
			DrawBalloonEffect(tgt);
	}

	originalDrawArcadeGame(data);
}

static void PatchCommonKiseki(PvGameTarget* target)
{
	float r, g, b;
	TargetStateEx* ex = GetTargetStateEx(target);

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

	if (state.chance_time.CheckTargetInRange(target->target_index))
	{
		for (int i = 0; i < 40; i++)
		{
			int32_t alpha = 0xFF000000;
			if (i >= 20)
			{
				if (i % 2 == 0)
					alpha = static_cast<int32_t>((1.0f - static_cast<float>(i - 20 + 2) / 20.0f) * 255) << 24;
				else
					alpha = target->kiseki[i - 1].color & 0xFF000000;
			}

			target->kiseki[i].uv.y = i % 2 != 0 ? 64.0f : 128.0f;
			target->kiseki[i].color = alpha | 0x00FFFFFF;
		}
	}
	else if (target->target_type >= TargetType_Custom)
	{
		uint32_t color = (uint8_t)(r * 255) |
			((uint8_t)(g * 255) << 8) |
			((uint8_t)(b * 255) << 16);

		for (int i = 0; i < 40; i++)
			target->kiseki[i].color = (target->kiseki[i].color & 0xFF000000) | (color & 0x00FFFFFF);
	}
}

static diva::vec2 GetLongKisekiOffset(TargetStateEx* ex)
{
	return ex->target_type == TargetType_TriangleLong ? diva::vec2(0.0f, 2.0f) : diva::vec2(0.0f, 0.0f);
}

static void CreateLongNoteKisekiBuffer(TargetStateEx* ex)
{
	ex->vertex_count_max = ex->length * KisekiRate * 2 + 2;
	if (ex->vertex_count_max % 2 != 0)
		ex->vertex_count_max += 3;

	ex->kiseki.resize(ex->vertex_count_max);

	// TODO: Calculate width based on long note length to prevent texture blurriness
	float uv_width  = 128.0f;
	float uv_height = 64.0f;
	float uv_pos_y  = uv_height / 2.0f;
	uint32_t color = IsSuddenEquipped(GetPVGameData()) ? 0x00FFFFFF : 0xFFFFFFFF;

	for (size_t i = 0; i < ex->vertex_count_max; i += 2)
	{
		diva::vec2 offset = GetLongKisekiOffset(ex);

		ex->kiseki[i].pos       = diva::vec3(GetScaledPosition(ex->kiseki_pos + offset), 0.0f);
		ex->kiseki[i].uv        = diva::vec2(uv_width, uv_pos_y);
		ex->kiseki[i].color     = color;
		ex->kiseki[i + 1].pos   = diva::vec3(GetScaledPosition(ex->kiseki_pos + offset), 0.0f);
		ex->kiseki[i + 1].uv    = diva::vec2(uv_width, uv_height);
		ex->kiseki[i + 1].color = color;
	}
}

static void UpdateLongNoteKiseki(PVGameArcade* data, TargetStateEx* ex, float dt)
{
	if (!ex->IsLongNoteStart() || ex->length <= 0.0f || !data || !ex)
		return;

	if (ex->kiseki.size() == 0)
		CreateLongNoteKisekiBuffer(ex);

	if (ex->vertex_count_max < 4)
		return;

	// NOTE: Push vertices out of the array for new ones to be inserted
	ex->kiseki_time += dt;
	size_t push_count = 0;

	while (ex->kiseki_time >= KisekiInterval)
	{
		for (size_t i = ex->vertex_count_max - 3; i > 0; i--)
			ex->kiseki[i + 2] = ex->kiseki[i];
		ex->kiseki[2] = ex->kiseki[0];

		push_count++;
		ex->kiseki_time -= KisekiInterval;
	}

	// NOTE: Snap kiseki to the target position when hitting too late
	// TODO: Fix kiseki when hitting too early too (F does it by simulating the button position until the time the target should be reached)
	if (ex->fix_long_kiseki)
	{
		if (ex->hit_time < 0.0f)
		{
			push_count += static_cast<size_t>(fabsf(ex->hit_time) / KisekiInterval);
			ex->fix_long_kiseki = false;
		}
	}

	const float uv_width = 128.0f;
	const float px_width = 7.2f;
	const diva::vec2 offset = GetLongKisekiOffset(ex);

	if (ex->org != nullptr && ex->flying_time_remaining > 0.0f)
		ex->alpha = ex->org->button_opacity;

	for (size_t i = 0; i < push_count; i++)
	{
		// TODO: Rename kiseki_dir to target_normal or something like that
		diva::vec2 size = ex->kiseki_dir * px_width;
		diva::vec2 left = GetScaledPosition(ex->kiseki_pos + offset + -size);
		diva::vec2 right = GetScaledPosition(ex->kiseki_pos + offset + size);
		uint32_t color = 0x00FFFFFF | (static_cast<int32_t>(ex->alpha * 255.0f) << 24);

		ex->kiseki[i * 2].pos       = diva::vec3(right, 0.0f);
		ex->kiseki[i * 2].color     = color;
		ex->kiseki[i * 2 + 1].pos   = diva::vec3(left, 0.0f);
		ex->kiseki[i * 2 + 1].color = color;
	}

	// NOTE: Update UV
	for (size_t i = 0; i < ex->vertex_count_max; i++)
		ex->kiseki[i].uv.x += dt * uv_width;
}

static void DrawLongNoteKiseki(TargetStateEx* ex)
{
	if (ex->vertex_count_max != 0)
	{
		int32_t sprite_id = 0x3AB;
		switch (ex->target_type)
		{
		case TargetType_TriangleLong:
			sprite_id = 1140037210; // SPR_GAM_EXTRA_KISEKI02
			break;
		case TargetType_CircleLong:
			sprite_id = 2660966235; // SPR_GAM_EXTRA_KISEKI03
			break;
		case TargetType_CrossLong:
			sprite_id = 1305045037; // SPR_GAM_EXTRA_KISEKI04
			break;
		case TargetType_SquareLong:
			sprite_id = 2946182585; // SPR_GAM_EXTRA_KISEKI05
			break;
		case TargetType_StarLong:
			sprite_id = 3052385779; // SPR_GAM_EXTRA_KISEKI06
			break;
		}

		DrawTriangles(ex->kiseki.data(), ex->vertex_count_max, 13, 7, sprite_id);
	}
}

static uint32_t GetBalloonSpriteId(const TargetStateEx* ex)
{
	constexpr uint32_t PlaystationStyle[4] = { 182, 181, 180, 183 };

	switch (ex->target_type)
	{
	case TargetType_TriangleRush:
	case TargetType_CircleRush:
	case TargetType_CrossRush:
	case TargetType_SquareRush:
	{
		// TODO: Add style logic
		int32_t base_id = ex->target_type - TargetType_TriangleRush;
		return PlaystationStyle[base_id];
	}
	case TargetType_StarRush:
		return 265324370;
	}

	return 0;
}

static void DrawBalloonEffectNote(uint32_t id, const diva::vec2& pos, const diva::vec2& scale, float opacity)
{
	SprArgs args;
	args.id = id;
	args.trans.x = pos.x;
	args.trans.y = pos.y;
	args.trans.z = 0.0f;
	args.scale.x = scale.x;
	args.scale.y = scale.y;
	args.scale.z = 1.0f;
	args.resolution_mode_sprite = 13;
	args.resolution_mode_sprite = 13;
	args.index = 0;
	args.layer = 0;
	args.priority = 12;
	args.attr = 0x400000; // SPR_ATTR_CTR_CC
	args.color[0] = 0xFF;
	args.color[1] = 0xFF;
	args.color[2] = 0xFF;
	args.color[3] = fminf(fmaxf(opacity * 255.0f, 0.0f), 255.0f);
	spr::DrawSprite(&args);
}

static void DrawBalloonEffect(TargetStateEx* ex)
{
	if (ex->bal_effect_aet == 0)
		return;

	AetComposition comp;
	aet::GetComposition(&comp, ex->bal_effect_aet);

	float scale = 1.0f + ex->bal_scale;

	for (const auto& [name, layout] : comp)
	{
		diva::vec2 pos = GetScaledPosition(ex->target_pos);
		pos.x += layout.position.x * scale;
		pos.y += layout.position.y * scale;

		// HACK: Use matrix data to determine the scale of the sprite.
		//       This will break if you use rotation on it!
		diva::vec2 spr_scale = { layout.matrix.row0.x, layout.matrix.row1.y };

		DrawBalloonEffectNote(
			GetBalloonSpriteId(ex),
			pos,
			spr_scale,
			layout.opacity
		);
	}
}

void InstallTargetHooks()
{
	INSTALL_HOOK(CreateTargetAetLayers);
	INSTALL_HOOK(UpdateTargets);
	INSTALL_HOOK(UpdateKiseki);
	INSTALL_HOOK(DrawKiseki);
	INSTALL_HOOK(DrawArcadeGame);
}