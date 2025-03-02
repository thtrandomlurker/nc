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
	for (int i = 0; i < Stick_Max; i++)
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

	for (int i = 0; i < Button_Max; i++)
	{
		buttons[i].tapped = buttons[i].down && prev[i].up;
		buttons[i].released = buttons[i].up && prev[i].down;
	}

	return true;
}

bool MacroState::IsStickFlicked(int32_t index) const
{
	if (index < 0 || index >= Stick_Max)
		return false;

	if (sticks[index].dist > 0.05)
		return fabsf(sticks[index].cur) >= sensivity;

	return false;
}

bool MacroState::IsStickPushed(int32_t index) const
{
	if (index < 0 || index >= Stick_Max)
		return false;

	return fabsf(sticks[index].cur) >= sensivity;
}

int32_t MacroState::GetStickFlicked() const
{
	for (int i = 0; i < Stick_Max; i++)
	{
		if (IsStickFlicked(i))
			return i;
	}

	return Stick_Max;
}

bool MacroState::GetStickDoubleFlicked(bool* both_flicked) const
{
	int32_t l = Stick_Max;
	int32_t r = Stick_Max;

	if (IsStickPushed(Stick_LX))
		l = Stick_LX;
	else if (IsStickPushed(Stick_LY))
		l = Stick_LY;

	if (IsStickPushed(Stick_RX))
		r = Stick_RX;
	else if (IsStickPushed(Stick_RY))
		r = Stick_RY;

	if (IsStickFlicked(l) || IsStickFlicked(r))
	{
		if (both_flicked != nullptr)
			*both_flicked = IsStickFlicked(l) && IsStickFlicked(r);

		return true;
	}

	return false;
}

bool MacroState::GetStarHit() const
{
	if (device != InputDevice_Keyboard)
		return GetStickFlicked() != Stick_Max;

	for (int32_t i = Button_L1; i < Button_Max; i++)
	{
		if (buttons[i].tapped)
			return true;
	}

	return false;
}

bool MacroState::GetDoubleStarHit(bool* both_flicked) const
{
	if (device != InputDevice_Keyboard)
		return GetStickDoubleFlicked(both_flicked);

	int32_t l = Button_Max;
	int32_t r = Button_Max;

	for (int32_t i = Button_L1; i <= Button_L4; i++)
	{
		if (buttons[i].down)
			l = i;
	}

	for (int32_t i = Button_R1; i <= Button_R4; i++)
	{
		if (buttons[i].down)
			r = i;
	}

	if (l != Button_Max && r != Button_Max)
	{
		if (buttons[l].tapped || buttons[r].tapped)
		{
			if (both_flicked != nullptr)
				*both_flicked = buttons[l].tapped && buttons[r].tapped;

			return true;
		}
	}

	return false;
}