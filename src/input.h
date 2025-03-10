#pragma once

#include <stdint.h>

enum Button : int32_t
{
	Button_Triangle    = 0,
	Button_Circle      = 1,
	Button_Cross       = 2,
	Button_Square      = 3,
	Button_Up          = 4,
	Button_Right       = 5,
	Button_Down        = 6,
	Button_Left        = 7,
	Button_L1          = 8,
	Button_L2          = 9,
	Button_L3          = 10,
	Button_L4          = 11,
	Button_R1          = 12,
	Button_R2          = 13,
	Button_R3          = 14,
	Button_R4          = 15,
	// Button_LStickUp    = 16,
	// Button_LStickDown  = 17,
	// Button_LStickLeft  = 18,
	// Button_LStickRight = 19,
	// Button_RStickUp    = 20,
	// Button_RStickDown  = 21,
	// Button_RStickLeft  = 22,
	// Button_RStickRight = 23,
	Button_LStick      = 24,
	Button_RStick      = 25,
	Button_Max,
	Button_None
};

enum StickAxis : int32_t
{
	StickAxis_LX = 0,
	StickAxis_LY = 1,
	StickAxis_RX = 2,
	StickAxis_RY = 3,
	StickAxis_Max
};

struct ButtonState
{
	struct StateInternal
	{
		bool down;
		bool up;
		bool tapped;
		bool released;
	} data[16];
	
	inline bool IsDown() const { return data[0].down; }
	inline bool IsTapped() const { return data[0].tapped; }
	inline bool IsUp() const { return data[0].up; }
	inline bool IsReleased() const { return data[0].released; }
	inline bool IsTappedInNearFrames() const { return data[0].tapped || data[1].tapped || data[2].tapped; }
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
	StickState sticks[StickAxis_Max];
	float sensivity;
	int32_t device;

	MacroState()
	{
		memset(buttons, 0, sizeof(buttons));
		memset(sticks, 0, sizeof(sticks));
		sensivity = 0.6f;
		device = InputDevice_Unknown;
	}

	bool Update(void* internal_handler, int32_t player_index);
	bool GetStarHit() const;
	bool GetDoubleStarHit(bool* both_flicked) const;
};