#include <common.h>
#include "note_common.h"

void PatchCommonKisekiColor(PvGameTarget* target)
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