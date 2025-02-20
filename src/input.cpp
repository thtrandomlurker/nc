#include <string.h>
#include "Helpers.h"
#include "input.h"

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

void GetCurrentInputState(void* handler, InputState* state)
{
	if (handler == nullptr || state == nullptr)
		return;

	// NOTE: Move current state to previous state before updating
	memcpy(state->prev, state->buttons, sizeof(state->buttons));

	int64_t key_states[8];
	for (int i = 0; i < 8; i++)
		key_states[i] = GetPvKeyStateDown(handler, i);

	UpdateButtonState(&state->buttons[Button_Triangle], key_states, ButtonIndex_Triangle, GameButton_Triangle);
	UpdateButtonState(&state->buttons[Button_Circle], key_states, ButtonIndex_Circle, GameButton_Circle);
	UpdateButtonState(&state->buttons[Button_Cross], key_states, ButtonIndex_Cross, GameButton_Cross);
	UpdateButtonState(&state->buttons[Button_Square], key_states, ButtonIndex_Square, GameButton_Square);
	UpdateButtonState(&state->buttons[Button_Up], key_states, ButtonIndex_Triangle, GameButton_Up);
	UpdateButtonState(&state->buttons[Button_Right], key_states, ButtonIndex_Circle, GameButton_Right);
	UpdateButtonState(&state->buttons[Button_Down], key_states, ButtonIndex_Cross, GameButton_Down);
	UpdateButtonState(&state->buttons[Button_Left], key_states, ButtonIndex_Square, GameButton_Left);
	UpdateButtonState(&state->buttons[Button_L1], key_states, ButtonIndex_L, GameButton_L1, GameButton_L2);
	UpdateButtonState(&state->buttons[Button_L2], key_states, ButtonIndex_L, GameButton_L3, GameButton_L4);
	UpdateButtonState(&state->buttons[Button_R1], key_states, ButtonIndex_R, GameButton_R1, GameButton_R2);
	UpdateButtonState(&state->buttons[Button_R2], key_states, ButtonIndex_R, GameButton_R3, GameButton_R4);

	for (int i = 0; i < Button_Max; i++)
	{
		state->buttons[i].tapped = state->buttons[i].down && state->prev[i].up;
		state->buttons[i].released = state->buttons[i].up && state->prev[i].down;
	}
}