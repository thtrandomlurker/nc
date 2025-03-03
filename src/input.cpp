#include <string.h>
#include "Helpers.h"
#include "diva.h"
#include "input.h"
#include "nc_log.h"

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

static void UpdateButtonState(ButtonState* state, const int64_t* buffer, int32_t index, int32_t mask, int32_t mask2 = -1)
{
	state->down = (buffer[index] & mask) != 0;
	if (mask2 != -1)
		state->down = state->down || ((buffer[index] & mask2) != 0);

	state->up = !state->down;
}

bool MacroState::Update(void* internal_handler, int32_t player_index)
{
	if (internal_handler == nullptr)
		return false;

	diva::InputState* diva_input = diva::GetInputState(player_index);
	device = diva_input->GetDevice();

	// NOTE: Update sticks
	for (int i = 0; i < StickAxis_Max; i++)
	{
		sticks[i].prev  = sticks[i].cur;
		sticks[i].cur   = diva_input->GetPosition(0x14 + i);
		sticks[i].delta = sticks[i].cur - sticks[i].prev;
		sticks[i].dist  = fabsf(sticks[i].cur) - fabsf(sticks[i].prev);
	}

	// NOTE: Update buttons
	memcpy(prev, buttons, sizeof(buttons));

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

	buttons[Button_LStick].down = fabsf(sticks[StickAxis_LX].cur) >= sensivity || fabsf(sticks[StickAxis_LY].cur) >= sensivity;
	buttons[Button_RStick].down = fabsf(sticks[StickAxis_RX].cur) >= sensivity || fabsf(sticks[StickAxis_RY].cur) >= sensivity;
	buttons[Button_LStick].up = !buttons[Button_LStick].down;
	buttons[Button_RStick].up = !buttons[Button_RStick].down;

	for (int i = 0; i < Button_Max; i++)
	{
		buttons[i].tapped = buttons[i].down && prev[i].up;
		buttons[i].released = buttons[i].up && prev[i].down;
	}

	return true;
}

bool MacroState::GetStarHit() const
{
	return buttons[Button_L1].tapped ||
		buttons[Button_L2].tapped ||
		buttons[Button_L3].tapped ||
		buttons[Button_L4].tapped ||
		buttons[Button_R1].tapped ||
		buttons[Button_R2].tapped ||
		buttons[Button_R3].tapped ||
		buttons[Button_R4].tapped ||
		buttons[Button_LStick].tapped ||
		buttons[Button_RStick].tapped;
}

bool MacroState::GetDoubleStarHit(bool* both_flicked) const
{
	const int32_t buttons_left[] = {
		Button_L1, Button_L2, Button_L3, Button_L4, Button_LStick, Button_RStick
	};

	const int32_t buttons_right[] = {
		Button_R1, Button_R2, Button_R3, Button_R4, Button_LStick, Button_RStick
	};

	const ButtonState* l = nullptr;
	const ButtonState* r = nullptr;

	for (int32_t i = 0; i < 6; i++)
		if (buttons[buttons_left[i]].down)
			l = &buttons[buttons_left[i]];

	for (int32_t i = 0; i < 6; i++)
		if (buttons[buttons_right[i]].down)
			r = &buttons[buttons_right[i]];

	if (l != nullptr && r != nullptr)
	{
		if (l->tapped || r->tapped)
		{
			if (both_flicked != nullptr)
				*both_flicked = l->tapped && r->tapped;

			return true;
		}
	}

	if (both_flicked != nullptr)
		*both_flicked = false;
	return false;
}