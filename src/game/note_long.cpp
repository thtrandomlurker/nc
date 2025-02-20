#include "note_long.h"

constexpr float LongKisekiWidth = 12.0f;
static const uint32_t LongKisekiIDs[] = {
	2547777446, // KISEKI02 - Triangle
	268038873,  // KISEKI03 - Circle
	3751393426, // KISEKI04 - Cross
	1382161087, // KISEKI05 - Square
	337203558,  // KISEKI06 - Star
};

void UpdateLongNoteKiseki(PVGameArcade* data, PvGameTarget* target, TargetStateEx* ex, float dt)
{
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
		float offset_x = ex->kiseki_dir.x * (LongKisekiWidth / 2.0f);
		float offset_y = ex->kiseki_dir.y * (LongKisekiWidth / 2.0f);

		diva::vec3 left = {
			ex->kiseki_pos.x - (ex->kiseki_dir.x * LongKisekiWidth) + offset_x,
			ex->kiseki_pos.y - (ex->kiseki_dir.y * LongKisekiWidth) + offset_y,
			0.0f
		};

		diva::vec3 right = {
			ex->kiseki_pos.x + (ex->kiseki_dir.x * LongKisekiWidth) + offset_x,
			ex->kiseki_pos.y + (ex->kiseki_dir.y * LongKisekiWidth) + offset_y,
			0.0f
		};

		/*
		if (ex->target_type == TargetType_TriangleLong)
		{
			left.y += 2.0f;
			right.y += 2.0f;
		}
		*/

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

void DrawLongNoteKiseki(TargetStateEx* ex)
{
	if (ex->vertex_count_max != 0)
	{
		int32_t sprite_id = 0x3AB;
		switch (ex->target_type)
		{
		case TargetType_TriangleLong:
			sprite_id = LongKisekiIDs[0];
			break;
		case TargetType_CircleLong:
			sprite_id = LongKisekiIDs[1];
			break;
		case TargetType_CrossLong:
			sprite_id = LongKisekiIDs[2];
			break;
		case TargetType_SquareLong:
			sprite_id = LongKisekiIDs[3];
			break;
		case TargetType_StarLong:
			sprite_id = LongKisekiIDs[4];
			break;
		}

		DrawTriangles(ex->kiseki.data(), ex->vertex_count_max, 13, 7, sprite_id);
	}
}