#include <initializer_list>
#include <util.h>
#include <input.h>
#include "common.h"

void AetElement::Ctrl() { }
void AetElement::Disp() { }

bool AetElement::SetLayer(std::string name, int32_t flags, int32_t prio, int32_t res_mode, std::string_view start_marker, std::string_view end_marker, const diva::vec3* pos)
{
	layer_name = name;

	args = { };
	args.scene_id = scene_id;
	args.layer_name = layer_name.c_str();
	args.start_marker = start_marker;
	args.end_marker = end_marker;
	args.flags = flags;
	args.index = 0;
	args.layer = 0;
	args.prio = prio;
	args.res_mode = res_mode;
	args.pos = (pos != nullptr) ? *pos : diva::vec3();

	handle = aet::Play(&args, handle);
	return handle != 0;
}

bool AetElement::SetLayer(std::string name, int32_t flags, int32_t prio, int32_t res_mode, int32_t action)
{
	layer_name = name;
	args = AetArgs(scene_id, layer_name.c_str(), prio, action);
	args.res_mode = res_mode;
	handle = aet::Play(&args, handle);
	return handle != 0;
}

std::optional<AetLayout> AetElement::GetLayout(std::string layer_name) const
{
	AetComposition comp;
	aet::GetComposition(&comp, handle);

	if (auto it = comp.find(prj::string(layer_name)); it != comp.end())
		return std::optional<AetLayout>(it->second);
	return std::nullopt;
}

void AetElement::SetPosition(const diva::vec3& pos)
{
	args.pos = pos;
	aet::SetPosition(handle, &pos);
}

void AetElement::SetOpacity(float opacity)
{
	args.color.w = opacity;
	aet::SetOpacity(handle, opacity);
}

void AetElement::SetLoop(bool loop)
{
	int32_t flags = args.flags & ~0x30000;
	args.flags = flags | (loop ? 0x10000 : 0x20000);
}

void AetElement::SetMarkers(const std::string& start_marker, const std::string& end_marker)
{
	args.start_marker = start_marker;
	args.end_marker = end_marker;
	handle = aet::Play(&args, handle);
}

bool AetElement::DrawSpriteAt(std::string layer_name, uint32_t sprite_id, int32_t prio) const
{
	auto layout = GetLayout(layer_name);
	if (!layout.has_value())
		return false;

	int32_t attr = 0x40000;
	if (util::StartsWith(layer_name, "p_"))
	{
		if      (util::EndsWith(layer_name, "_c"))  attr = 0x400000; // NOTE: These two are up top because they are
		else if (util::EndsWith(layer_name, "_rt")) attr = 0x100000; //       the most commonly used.
		else if (util::EndsWith(layer_name, "_ct")) attr = 0x80000;
		else if (util::EndsWith(layer_name, "_lc")) attr = 0x200000;
		else if (util::EndsWith(layer_name, "_rc")) attr = 0x800000;
		else if (util::EndsWith(layer_name, "_lb")) attr = 0x1000000;
		else if (util::EndsWith(layer_name, "_cb")) attr = 0x2000000;
		else if (util::EndsWith(layer_name, "_rb")) attr = 0x4000000;
	}

	SprArgs spr_args = { };
	spr_args.id = sprite_id;
	spr_args.attr = attr;
	spr_args.trans = layout.value().position;
	spr_args.scale = { layout.value().matrix.row0.x, layout.value().matrix.row1.y, 1.0f };
	memset(spr_args.color, 0xFF, 4);
	spr_args.color[3] = static_cast<uint8_t>(fminf(layout.value().opacity * 255.0f, 255.0f));
	spr_args.resolution_mode_screen = args.res_mode;
	spr_args.resolution_mode_sprite = args.res_mode;
	spr_args.priority = args.prio + prio;

	spr::DrawSprite(&spr_args);
	return true;
}

void AetElement::DeleteHandle()
{
	aet::Stop(&handle);
}

void AetControl::AllowInputsWhenBlocked(bool allow) { abs_input = allow; }
void AetControl::SetFocus(bool focus) { focused = focus; }

void AetControl::OnActionPressed(int32_t action) { }
void AetControl::OnActionPressedOrRepeat(int32_t action) { }

void AetControl::Ctrl()
{
	auto dispatchAction = [&](int32_t action, std::initializer_list<int32_t> keys)
	{
		diva::InputState* is = diva::GetInputState(0);
		bool tapped = false;
		bool repeat = false;

		for (int32_t key : keys)
		{
			if (!abs_input || !is->IsInputBlocked())
			{
				tapped = tapped || is->IsButtonTapped(key);
				repeat = repeat || is->IsButtonTappedOrRepeat(key);
			}
			else
			{
				tapped = tapped || is->IsButtonTappedAbs(key);
				repeat = repeat || nc::IsButtonTappedOrRepeat(is, key);
			}
		}

		if (tapped)
			OnActionPressed(action);

		if (repeat)
			OnActionPressedOrRepeat(action);
	};

	if (IsFocused())
	{
		dispatchAction(KeyAction_MoveUp, { 3 });
		dispatchAction(KeyAction_MoveDown, { 4 });
		dispatchAction(KeyAction_MoveLeft, { 5 });
		dispatchAction(KeyAction_MoveRight, { 6 });
		dispatchAction(KeyAction_Enter, { 10 });
		dispatchAction(KeyAction_Cancel, { 9 });
		dispatchAction(KeyAction_SwapLeft, { 11 });
		dispatchAction(KeyAction_SwapRight, { 12 });
		dispatchAction(KeyAction_Preview, { 7 });
	}

	AetElement::Ctrl();
}

void HorizontalSelector::OnActionPressedOrRepeat(int32_t action)
{
	switch (action)
	{
	case KeyAction_MoveLeft:
		ChangeValue(-1);
		sound::PlaySelectSE();
		break;
	case KeyAction_MoveRight:
		ChangeValue(1);
		sound::PlaySelectSE();
		break;
	}
}

void HorizontalSelector::OnActionPressed(int32_t action)
{
	switch (action)
	{
	case KeyAction_Enter:
	case KeyAction_Preview:
		if (preview_notify.has_value())
		{
			preview_notify.value()(this, -1, extra_data);
			enter_anim_state = 0;
		}
		break;
	}
}

void HorizontalSelector::Ctrl()
{
	AetControl::Ctrl();

	if (enter_anim_state == 0)
	{
		SetLoop(false);
		SetMarkers("st_sp", "ed_sp");
		enter_anim_state = 1;
	}
	else if (enter_anim_state == 1)
	{
		if (Ended())
		{
			SetLoop(true);
			SetMarkers("st_lp", "ed_lp");
			enter_anim_state = -1;
		}
	}

	if (IsFocused() != focused_old)
	{
		SetLoop(true);
		enter_anim_state = -1;

		if (IsFocused())
			SetMarkers("st_lp", "ed_lp");
		else
			SetMarkers("st_in", "ed_in");

		focused_old = IsFocused();
	}
}

void HorizontalSelector::Disp()
{
	std::optional<AetLayout> layout = GetLayout("p_nc_setting_txt2_sel_c");
	if (!layout.has_value())
		layout = GetLayout("p_nc_setting_txt2_c");

	if (layout.has_value())
	{
		diva::vec3 pos = layout.value().position;
		const AetArgs& args = GetArgs();

		uint32_t base_color = util::ColorF32I32(1.0f, 1.0f, 1.0f, layout.value().opacity * text_opacity);
		uint32_t fill_color = util::ColorF32I32(1.0f, 1.0f, 1.0f, layout.value().opacity * text_opacity);

		FontInfo font = { };
		TextArgs text_args;
		text_args.max_width = squish_text ? layout.value().width * layout.value().matrix.row0.x : -1.0f;
		text_args.print_work.line_origin = diva::vec2(args.pos.x + pos.x, args.pos.y + pos.y);
		text_args.print_work.text_current = text_args.print_work.line_origin;
		text_args.print_work.res_mode = args.res_mode;
		text_args.print_work.prio = args.prio + 1;
		text_args.print_work.SetColor(base_color, fill_color);
		text_args.print_work.font = spr::GetLocaleFont(&font, bold_text ? 19 : 18, true);
		spr::SetFontSize(&font, 36.0f * font_scale, 38.0f * font_scale);

		spr::DrawTextA(&text_args, TextFlags_AutoAlignCenter, GetSelectedValue().c_str());
	}
}

std::string HorizontalSelectorMulti::GetSelectedValue()
{
	if (selected_index < 0 || selected_index >= values.size() || values.empty())
		return "(NULL)";
	return values[selected_index];
}

void HorizontalSelectorMulti::ChangeValue(int32_t dir)
{
	if (values.empty())
		return;

	selected_index += dir;
	if (selected_index < 0)
		selected_index = values.size() - 1;
	else if (selected_index >= values.size())
		selected_index = 0;

	if (notify.has_value())
		notify.value()(selected_index, values[selected_index]);
}