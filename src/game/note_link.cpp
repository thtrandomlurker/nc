#include "note_link.h"

static diva::vec2 CalculateDirection(diva::vec2& start, diva::vec2& end, bool rot)
{
	diva::vec2 dir_delta = end - start;
	if (rot)
		return (dir_delta / dir_delta.length()).rotated(1.570796);
	return dir_delta / dir_delta.length();
}


void UpdateLinkStar(PVGameArcade* data, TargetStateEx* chain, float dt)
{
	// NOTE: Update link chain and determine which step we're currently on
	//
	TargetStateEx* current = nullptr;
	TargetStateEx* next = nullptr;

	for (TargetStateEx* ex = chain; ex != nullptr; ex = ex->next)
	{
		// NOTE: Reset state
		ex->current_step = false;
		ex->kiseki_pos = ex->target_pos;

		if (ex->next != nullptr)
		{
			ex->kiseki_dir = CalculateDirection(ex->target_pos, ex->next->target_pos, true);
			ex->kiseki_dir_norot = CalculateDirection(ex->target_pos, ex->next->target_pos, false);
		}

		// NOTE: Update target
		if (ex->flying_time_max > 0.0f)
		{
			// NOTE: Calculate target note scale. Link Stars are not removed after the target is hit,
			//       it just does the shrinking out animation for every piece (except the end one)
			if (ex->flying_time_remaining <= data->cool_late_window && ex->target_aet != 0)
			{
				float scale = 1.0f - (ex->flying_time_remaining / data->sad_late_window);
				if (scale >= 0.0f)
				{
					diva::vec3 v = { scale, scale, 1.0f };
					diva::aet::SetScale(ex->target_aet, &v);
				}
				else
					diva::aet::Stop(&ex->target_aet);
			}

			// NOTE: Calculate target note frame time
			if (ex->target_aet != 0)
			{
				float frame = (ex->flying_time_max - ex->flying_time_remaining) / ex->flying_time_max * 360.0f;
				frame = fminf(fmaxf(frame, 0.0f), 360.0f);
				diva::aet::SetFrame(ex->target_aet, frame);
				diva::aet::SetPlay(ex->target_aet, false);
			}

			if (ex->flying_time_remaining <= 0.0f)
			{
				current = ex;
				next = ex->next;
			}

			// NOTE: Calculate alpha
			switch (ex->step_state)
			{
			case LinkStepState_None:
				if (ex->next != nullptr && ex->next->org != nullptr)
				{
					ex->kiseki_time = 0.3f;
					ex->alpha = 0.0f;
					ex->step_state = LinkStepState_Normal;
				}

				break;
			case LinkStepState_Normal:
				ex->alpha = 1.0f - ex->kiseki_time / 0.3f;
				ex->kiseki_time -= dt;

				if (ex->alpha >= 1.0f)
				{
					ex->alpha = 1.0f;
					ex->kiseki_time = 0.0f;
					ex->step_state = LinkStepState_Wait;
				}

				break;
			case LinkStepState_GlowStart:
				ex->alpha = 1.0f - (ex->kiseki_time / 0.2f);
				ex->kiseki_time -= dt;

				if (ex->kiseki_time <= 0.0f)
				{
					ex->kiseki_time = 0.5f;
					ex->alpha = 1.0f;
					ex->step_state = LinkStepState_Glow;
				}

				break;
			case LinkStepState_Glow:
				if (ex->kiseki_time <= 0.0f)
				{
					ex->kiseki_time = 0.2f;
					ex->alpha = 1.0f;
					ex->step_state = LinkStepState_GlowEnd;
				}

				ex->kiseki_time -= dt;
				break;
			case LinkStepState_GlowEnd:
				ex->alpha = ex->kiseki_time / 0.2f;
				ex->kiseki_time -= dt;

				if (ex->kiseki_time <= 0.0f)
				{
					ex->kiseki_time = 0.0f;
					ex->alpha = 0.0f;
					ex->step_state = LinkStepState_Idle;
				}
				break;
			case LinkStepState_Wait:
				if (chain->IsChainSucessful())
				{
					// NOTE: Begin the glow animation
					//
					ex->kiseki_time = 0.2f;
					ex->alpha = 0.0f;
					ex->step_state = LinkStepState_GlowStart;
				}
				else if (ex->next != nullptr)
				{
					// NOTE: Hide kiseki after the button has reached the destination
					//
					if (ex->next->flying_time_max > 0.0f && ex->next->flying_time_remaining <= 0.0f)
					{
						ex->kiseki_time = 0.0f;
						ex->alpha = 0.0f;
					}
				}

				break;
			}

			ex->flying_time_remaining -= dt;
		}
	}

	if (current != nullptr)
	{
		// NOTE: Set the current position of the button
		if (!current->link_end && next != nullptr)
		{
			// NOTE: Initialize state
			if (current->delta_time_max <= 0.0f)
			{
				current->delta_time_max = next->flying_time_remaining;
				current->delta_time = current->delta_time_max;
			}

			current->current_step = true;

			// NOTE: Calculate the position
			float percentage = (current->delta_time_max - current->delta_time) / current->delta_time_max;
			diva::vec2 delta = next->target_pos - current->target_pos;
			diva::vec2 pos = current->target_pos + delta * percentage;

			// NOTE: Update kiseki data
			current->kiseki_pos = pos;
			current->kiseki_dir = CalculateDirection(pos, next->target_pos, true);
			current->kiseki_dir_norot = CalculateDirection(pos, next->target_pos, false);

			// NOTE: Set the position
			diva::vec3 pos_3 = { pos.x, pos.y, 0.0f };
			diva::GetScaledPosition((diva::vec2*)&pos_3, (diva::vec2*)&pos_3);
			diva::aet::SetPosition(chain->button_aet, &pos_3);

			// NOTE: Reset button frame to prevent it from scaling out
			diva::aet::SetFrame(chain->button_aet, 0.0f);

			current->delta_time -= dt;
		}
		else if (current->link_end)
		{
			if (!current->link_ending)
			{
				if (current->flying_time_remaining <= data->cool_late_window)
				{
					diva::aet::SetFrame(chain->button_aet, 360.0f);
					current->link_ending = true;
				}
			}
			else
			{
				diva::aet::StopOnEnded(&chain->button_aet);
			}
		}
	}
}

void UpdateLinkStarKiseki(PVGameArcade* data, TargetStateEx* chain, float dt)
{
	const float width = 12.0f;

	for (TargetStateEx* ex = chain; ex != nullptr; ex = ex->next)
	{
		if (ex->link_end || ex->next->org == nullptr)
			continue;

		ex->vertex_count_max = 20;
		ex->kiseki.resize(ex->vertex_count_max);

		// NOTE: Calculate color value for alpha
		uint32_t color = 0x00FFFFFF;
		color |= static_cast<uint32_t>(ex->alpha * 255.0f) << 24;

		// NOTE: Calculate position data
		diva::vec2 center = ex->kiseki_dir * width / 2.0f;
		diva::vec2 delta = (ex->next->target_pos - ex->kiseki_pos) / 10.0f;

		// NOTE: Calculate UV data
		diva::vec2 uv_offset = { 7.0f, 0.0f };
		float uv_step_x = 130.0f / 10.0f;
		float uv_size_y = 32.0f;

		for (int i = 0; i < 20; i += 2)
		{
			float step = static_cast<float>(i / 2);
			if (step > 0.0f)
				step += 1.0f;

			ex->kiseki[i].pos.x = ex->kiseki_pos.x + delta.x * step + ex->kiseki_dir.x * width;
			ex->kiseki[i].pos.y = ex->kiseki_pos.y + delta.y * step + ex->kiseki_dir.y * width;
			ex->kiseki[i].pos.z = 0.0f;
			ex->kiseki[i].uv.x = uv_offset.x + uv_step_x * step;
			ex->kiseki[i].uv.y = uv_offset.y + uv_size_y;
			ex->kiseki[i].color = color;

			ex->kiseki[i + 1].pos.x = ex->kiseki_pos.x + delta.x * step - ex->kiseki_dir.x * width;
			ex->kiseki[i + 1].pos.y = ex->kiseki_pos.y + delta.y * step - ex->kiseki_dir.y * width;
			ex->kiseki[i + 1].pos.z = 0.0f;
			ex->kiseki[i + 1].uv.x = uv_offset.x + uv_step_x * step;
			ex->kiseki[i + 1].uv.y = uv_offset.y;
			ex->kiseki[i + 1].color = color;

			// NOTE: Scale position
			diva::GetScaledPosition((diva::vec2*)&ex->kiseki[i].pos, (diva::vec2*)&ex->kiseki[i].pos);
			diva::GetScaledPosition((diva::vec2*)&ex->kiseki[i + 1].pos, (diva::vec2*)&ex->kiseki[i + 1].pos);
		}
	}
}