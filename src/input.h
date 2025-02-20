#pragma once

#include <stdint.h>

enum Button : int32_t
{
	Button_Triangle = 0,
	Button_Circle   = 1,
	Button_Cross    = 2,
	Button_Square   = 3,
	Button_Up       = 4,
	Button_Right    = 5,
	Button_Down     = 6,
	Button_Left     = 7,
	Button_L1       = 8,
	Button_L2       = 9,
	Button_R1       = 10,
	Button_R2       = 11,
	Button_Max,
	Button_None
};

struct ButtonState
{
	bool down;
	bool up;
	bool tapped;
	bool released;
};

struct InputState
{
	ButtonState buttons[Button_Max];
	ButtonState prev[Button_Max];
};

void GetCurrentInputState(void* handler, InputState* state);