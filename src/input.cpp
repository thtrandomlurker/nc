#include <string.h>
#include "hooks.h"
#include "diva.h"
#include "input.h"
#include "nc_log.h"

constexpr int32_t NearFramesBaseCount = 3;
constexpr float NearFramesBaseRate = 60.0f;

enum ButtonIndex : int32_t
{
	ButtonIndex_Circle = 0,
	ButtonIndex_Cross = 1,
	ButtonIndex_Triangle = 2,
	ButtonIndex_Square = 3,
	ButtonIndex_L = 4,
	ButtonIndex_05 = 5,
	ButtonIndex_06 = 6,
	ButtonIndex_R = 7
};

enum GameButton : int32_t
{
	GameButton_Triangle = 1,
	GameButton_Up = 2,
	GameButton_Square = 4,
	GameButton_Left = 8,
	GameButton_Cross = 16,
	GameButton_Down = 32,
	GameButton_Circle = 64,
	GameButton_Right = 128,
	GameButton_L1 = 256,   // Q
	GameButton_L2 = 512,   // U
	GameButton_L3 = 1024,  // Left Arrow
	GameButton_L4 = 2048,  // R
	GameButton_R1 = 4096,  // E
	GameButton_R2 = 8192,  // O
	GameButton_R3 = 16384, // Right Arrow
	GameButton_R4 = 32768, // Y
};

static FUNCTION_PTR(int64_t, __fastcall, GetPvKeyStateDown, 0x140274930, void* handler, int32_t key);
static FUNCTION_PTR(int64_t, __fastcall, GetPvKeyStateTapped, 0x140274960, void* handler, int32_t key);

bool ButtonState::IsTappedInNearFrames() const
{
	int32_t lookup_count = NearFramesBaseCount * (game::GetFramerate() / NearFramesBaseRate);
	bool mask = false;

	for (int32_t i = 0; i < lookup_count; i++)
	{
		if (i >= MaxKeepStates)
			break;

		mask = mask || data[i].tapped;
	}

	return mask;
}

static void UpdateButtonState(ButtonState* state, const int64_t* buffer, int32_t index, int32_t mask)
{
	state->data[0].down = (buffer[index] & mask) != 0;
	state->data[0].up = !state->data[0].down;
}

bool MacroState::Update(void* internal_handler, int32_t player_index)
{
	if (internal_handler == nullptr)
		return false;

	diva::InputState* diva_input = diva::GetInputState(player_index);
	device = diva_input->GetDevice();

	// NOTE: Update buttons
	for (int32_t i = 0; i < Button_Max; i++)
		buttons[i].Push();

	int64_t key_states[8];
	for (int i = 0; i < 8; i++)
		key_states[i] = GetPvKeyStateDown(internal_handler, i);

	UpdateButtonState(&buttons[Button_Triangle], key_states, ButtonIndex_Triangle, GameButton_Triangle);
	UpdateButtonState(&buttons[Button_Circle], key_states, ButtonIndex_Circle, GameButton_Circle);
	UpdateButtonState(&buttons[Button_Cross], key_states, ButtonIndex_Cross, GameButton_Cross);
	UpdateButtonState(&buttons[Button_Square], key_states, ButtonIndex_Square, GameButton_Square);
	UpdateButtonState(&buttons[Button_Up], key_states, ButtonIndex_Triangle, GameButton_Up);
	UpdateButtonState(&buttons[Button_Right], key_states, ButtonIndex_Circle, GameButton_Right);
	UpdateButtonState(&buttons[Button_Down], key_states, ButtonIndex_Cross, GameButton_Down);
	UpdateButtonState(&buttons[Button_Left], key_states, ButtonIndex_Square, GameButton_Left);
	UpdateButtonState(&buttons[Button_L1], key_states, ButtonIndex_L, GameButton_L1);
	UpdateButtonState(&buttons[Button_L2], key_states, ButtonIndex_L, GameButton_L2);
	UpdateButtonState(&buttons[Button_L3], key_states, ButtonIndex_L, GameButton_L3);
	UpdateButtonState(&buttons[Button_L4], key_states, ButtonIndex_L, GameButton_L4);
	UpdateButtonState(&buttons[Button_R1], key_states, ButtonIndex_R, GameButton_R1);
	UpdateButtonState(&buttons[Button_R2], key_states, ButtonIndex_R, GameButton_R2);
	UpdateButtonState(&buttons[Button_R3], key_states, ButtonIndex_R, GameButton_R3);
	UpdateButtonState(&buttons[Button_R4], key_states, ButtonIndex_R, GameButton_R4);

	for (int i = 0; i < Button_Max; i++)
	{
		buttons[i].data[0].tapped = buttons[i].data[0].down && buttons[i].data[1].up;
		buttons[i].data[0].released = buttons[i].data[0].up && buttons[i].data[1].down;
	}

	UpdateSticks(diva_input);
	return true;
}

void MacroState::UpdateSticks(diva::InputState* input_state)
{
	auto checkDeadzoned = [&](const diva::vec2& pos)
	{
		return pos.x <= stick_deadzone.x && pos.y <= stick_deadzone.y
			&& pos.x >= -stick_deadzone.x && pos.y >= -stick_deadzone.y;
	};

	auto updateStick = [&](int32_t index)
	{
		StickState* state = &sticks[index];
		diva::vec2 pos = {
			input_state->GetPosition(0x14 + index * 2 + 0),
			input_state->GetPosition(0x14 + index * 2 + 1)
		};

		state->prev_distance = state->distance;
		state->distance = checkDeadzoned(pos) ? 0.0f : pos.length();
		state->returning = state->distance < state->prev_distance;
		state->flicked = false;

		if (!state->returning)
		{
			state->flicked = state->distance >= sensivity && !state->flick_block;
			if (state->flicked)
				state->flick_block = true;
		}
		else
		{
			if (state->distance < sensivity)
				state->flick_block = false;
		}
	};

	auto updateStickButtonState = [&](int32_t index)
	{
		StickState* state = &sticks[index];
		auto& button = buttons[Button_LStick + index].data[0];

		button.down = state->distance >= sensivity;
		button.up = !button.down;
		button.tapped = state->flicked;
		// button.released = !button.tapped;
	};

	updateStick(Stick_L);
	updateStick(Stick_R);
	updateStickButtonState(Stick_L);
	updateStickButtonState(Stick_R);
}

bool MacroState::GetStarHit() const
{
	if (diva::GetInputState(0)->GetDevice() != InputDevice_Keyboard)
		return buttons[Button_LStick].IsTapped() || buttons[Button_RStick].IsTapped();

	return buttons[Button_L1].IsTapped() ||
		buttons[Button_L2].IsTapped() ||
		buttons[Button_L3].IsTapped() ||
		buttons[Button_L4].IsTapped() ||
		buttons[Button_R1].IsTapped() ||
		buttons[Button_R2].IsTapped() ||
		buttons[Button_R3].IsTapped() ||
		buttons[Button_R4].IsTapped();
}

bool MacroState::GetDoubleStarHit() const
{
	if (diva::GetInputState(0)->GetDevice() != InputDevice_Keyboard)
	{
		return (buttons[Button_LStick].IsTapped() && buttons[Button_RStick].IsTappedInNearFrames()) ||
			(buttons[Button_RStick].IsTapped() && buttons[Button_LStick].IsTappedInNearFrames());
	}

	const int32_t buttons_left[] = {
		Button_L1, Button_L2, Button_L3, Button_L4
	};

	const int32_t buttons_right[] = {
		Button_R1, Button_R2, Button_R3, Button_R4
	};

	const ButtonState* l = nullptr;
	const ButtonState* r = nullptr;

	for (int32_t i = 0; i < 4; i++)
		if (buttons[buttons_left[i]].IsDown())
			l = &buttons[buttons_left[i]];

	for (int32_t i = 0; i < 4; i++)
		if (buttons[buttons_right[i]].IsDown())
			r = &buttons[buttons_right[i]];

	if (l != nullptr && r != nullptr)
	{
		return (l->IsTapped() && r->IsTappedInNearFrames()) ||
			(r->IsTapped() && l->IsTappedInNearFrames());
	}

	return false;
}