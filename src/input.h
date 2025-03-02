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
	Button_L3       = 10,
	Button_L4       = 11,
	Button_R1       = 12,
	Button_R2       = 13,
	Button_R3       = 14,
	Button_R4       = 15,
	Button_Max,
	Button_None
};

enum Stick : int32_t
{
	Stick_LX = 0,
	Stick_LY = 1,
	Stick_RX = 2,
	Stick_RY = 3,
	Stick_Max
};

struct ButtonState
{
	bool down;
	bool up;
	bool tapped;
	bool released;
};

struct StickState
{
	float prev;
	float cur;
	float delta;
	float dist;
};

struct MacroState
{
	ButtonState buttons[Button_Max];
	ButtonState prev[Button_Max];
	StickState sticks[4];
	float sensivity;
	int32_t device;

	MacroState()
	{
		memset(buttons, 0, sizeof(buttons));
		memset(prev, 0, sizeof(prev));
		memset(sticks, 0, sizeof(sticks));
		sensivity = 0.8f;
		device = InputDevice_Unknown;
	}

	bool Update(void* internal_handler, int32_t player_index);
	bool IsStickFlicked(int32_t index) const;
	bool IsStickPushed(int32_t index) const;
	int32_t GetStickFlicked() const;
	bool GetStickDoubleFlicked(bool* both_flicked) const;

	bool GetStarHit() const;
	bool GetDoubleStarHit(bool* both_flicked) const;
};