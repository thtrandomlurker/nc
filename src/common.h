#pragma once

#include "diva.h"
#include "megamix.h"
#include "input.h"

enum TargetType : int32_t
{
	// FT
	TargetType_Triangle       = 0,
	TargetType_Circle         = 1,
	TargetType_Cross          = 2,
	TargetType_Square         = 3,
	TargetType_TriangleHold   = 4,
	TargetType_CircleHold     = 5,
	TargetType_CrossHold      = 6,
	TargetType_SquareHold     = 7,
	TargetType_Random         = 8,
	TargetType_RandomHold     = 9,
	TargetType_Previous       = 10,
	TargetType_0B             = 11,
	TargetType_SlideL         = 12,
	TargetType_SlideR         = 13,
	TargetType_0E             = 14,
	TargetType_ChainslideL    = 15,
	TargetType_ChainslideR    = 16,
	TargetType_11             = 17,
	TargetType_ChanceTriangle = 18,
	TargetType_ChanceCircle   = 19,
	TargetType_ChanceCross    = 20,
	TargetType_ChanceSquare   = 21,
	TargetType_16             = 22,
	TargetType_ChanceSlideL   = 23,
	TargetType_ChanceSlideR   = 24,

	// X (these are their actual IDs)
	TargetType_TriangleRush   = 25,
	TargetType_CircleRush     = 26,
	TargetType_CrossRush      = 27,
	TargetType_SquareRush     = 28,

	// PSP / F / F 2nd (Changed IDs so they don't overlap base game notes)
	TargetType_UpW            = 29,
	TargetType_RightW         = 30,
	TargetType_DownW          = 31,
	TargetType_LeftW          = 32,
	TargetType_TriangleLong   = 33,
	TargetType_CircleLong     = 34,
	TargetType_CrossLong      = 35,
	TargetType_SquareLong     = 36,
	TargetType_Star           = 37, 
	TargetType_StarLong       = 38, // NOTE: Unused F mechanic, should I implement this?
	TargetType_StarW          = 39,
	TargetType_ChanceStar     = 40,
	TargetType_LinkStar       = 41,
	TargetType_LinkStarEnd    = 42,
	TargetType_StarRush       = 43,

	TargetType_Max,
	TargetType_Custom = 25
};

enum SEState : int32_t
{
	SEState_Idle = 0,
	SEState_Looping = 1,
	SEState_FailRelease = 2,
	SEState_SuccessRelease = 3
};

struct TargetStateEx
{
	// NOTE: Static data; Information about the target.
	TargetStateEx* prev = nullptr;
	TargetStateEx* next = nullptr;
	int32_t target_type = -1;
	int32_t target_index = 0;
	int32_t sub_index = 0;
	float length = 0.0f;
	bool long_end = false;
	bool link_start = false;
	bool link_step = false;
	bool link_end = false;

	// NOTE: Gameplay state
	ButtonState* hold_button = nullptr;
	PvGameTarget* org = nullptr;
	int32_t force_hit_state = HitState_None;
	float length_remaining = 0.0f;
	float kiseki_time = 0.0f;
	bool holding = false;
	bool success = false; // NOTE: If this note is a chance star, this determines if it's successful or not

	// NOTE: Visual info for long notes. This is kind of a workaround as to not mess too much
	//       with the vanilla game structs.
	// PS:   Button aet handle isn't needed because this is only used for when the player is
	//       holding the button, so there isn't a button icon.
	int32_t target_aet = 0;
	diva::vec2 kiseki_pos = { }; // NOTE: Position where the kiseki will be updated from
	diva::vec2 kiseki_dir = { }; // NOTE: Direction of the note
	std::vector<SpriteVertex> kiseki;
	size_t vertex_count_max = 0;

	// NOTE: Sound effect state
	//
	int32_t se_state = SEState_Idle;
	int32_t se_queue = -1;
	std::string se_name = "";

	inline void ResetPlayState()
	{
		hold_button = nullptr;
		org = nullptr;
		force_hit_state = HitState_None;
		length_remaining = length;
		kiseki_time = 0.0f;
		holding = false;
		success = false;
		diva::aet::Stop(&target_aet);
		kiseki_pos = { 0.0f, 0.0f };
		kiseki_dir = { 0.0f, 0.0f };
		kiseki.clear();
		vertex_count_max = 0;
		ResetSE();
	}

	inline void ResetSE()
	{
		if (se_state != SEState_Idle && se_queue != -1 && !se_name.empty())
			diva::sound::ReleaseCue(se_queue, se_name.c_str(), true);

		se_state = SEState_Idle;
		se_queue = -1;
		se_name.clear();
	}
};

struct StateEx
{
	TargetStateEx* start_targets_ex[4];
	int32_t start_target_count;
	FileHandler file_handler;
	int32_t file_state;
	bool dsc_loaded;
	bool files_loaded;
	std::vector<TargetStateEx> target_ex;

	inline void ResetPlayState()
	{
		memset(start_targets_ex, 0, sizeof(start_targets_ex));
		start_target_count = 0;
		for (TargetStateEx& ex : target_ex)
			ex.ResetPlayState();
	}
};

// NOTE: Constants
constexpr uint32_t AetSetID = 16014000;
constexpr uint32_t AetSceneID = 16014001;
constexpr uint32_t SprSetID = 3068121241;

constexpr float KisekiRate = 60.0f; // NOTE: Framerate where the long note kiseki will be updated

// NOTE: Global state
inline InputState input_state = { };
inline StateEx state = { };

// NOTE: Helper functions

static inline bool IsLongNote(int32_t type)
{ 
	return type >= TargetType_TriangleLong && type <= TargetType_SquareLong;
}

static inline bool CheckWindow(float time, float early, float late)
{
	return time >= late && time <= early;
}

static inline TargetStateEx* GetTargetStateEx(int32_t index, int32_t sub_index = 0)
{
	for (TargetStateEx& ex : state.target_ex)
		if (ex.target_index == index && ex.sub_index == sub_index)
			return &ex;
	return nullptr;
}