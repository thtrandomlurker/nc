#pragma once

#include <list>
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

enum LinkStepState : int32_t
{
	LinkStepState_None      = 0,
	LinkStepState_FadeIn    = 1,
	LinkStepState_Normal    = 2,
	LinkStepState_GlowStart = 3,
	LinkStepState_Glow      = 4,
	LinkStepState_GlowEnd   = 5,
	LinkStepState_Wait      = 6,
	LinkStepState_Idle      = 7
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
	int32_t hit_state = HitState_None;
	float flying_time_max = 0.0f;
	float flying_time_remaining = 0.0f;
	float delta_time_max = 0.0f;
	float delta_time = 0.0f;
	float length_remaining = 0.0f;
	float kiseki_time = 0.0f;
	float alpha = 0.0f;
	bool holding = false;
	bool success = false; // NOTE: If this note is a chance star, this determines if it's successful or not
	bool current_step = false; // NOTE: If this target is the current step of the link star chain
	int32_t step_state = LinkStepState_None;
	bool link_ending = false;

	// NOTE: Visual info for long notes. This is kind of a workaround as to not mess too much
	//       with the vanilla game structs.
	// PS:   Button aet handle isn't needed because this is only used for when the player is
	//       holding the button, so there isn't a button icon.
	int32_t target_aet = 0;
	int32_t button_aet = 0;
	diva::vec2 target_pos = { };
	diva::vec2 kiseki_pos = { }; // NOTE: Position where the kiseki will be updated from
	diva::vec2 kiseki_dir = { }; // NOTE: Direction of the note
	diva::vec2 kiseki_dir_norot = { };
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
		hit_state = HitState_None;
		flying_time_max = 0.0f;
		flying_time_remaining = 0.0f;
		delta_time_max = 0.0f;
		delta_time = 0.0f;
		length_remaining = length;
		kiseki_time = 0.0f;
		alpha = 0.0f;
		holding = false;
		success = false;
		current_step = false;
		step_state = LinkStepState_None;
		link_ending = false;
		diva::aet::Stop(&target_aet);
		diva::aet::Stop(&button_aet);
		kiseki_pos = { 0.0f, 0.0f };
		kiseki_dir = { 0.0f, 0.0f };
		kiseki_dir_norot = { 0.0f, 0.0f };
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

	inline bool IsChainSucessful()
	{
		bool cond = true;
		for (TargetStateEx* ex = this; ex != nullptr; ex = ex->next)
			cond = cond && (ex->hit_state >= HitState_Cool && ex->hit_state <= HitState_Sad);

		return cond;
	}
};

struct ChanceState
{
	int32_t first_target_index = -1;
	int32_t last_target_index = -1;
	int32_t targets_hit = 0;

	inline void ResetPlayState()
	{
		targets_hit = 0;
	}

	inline int32_t GetTargetCount() const
	{
		if (first_target_index != -1 && last_target_index != -1)
			return last_target_index - first_target_index + 1;
		return 0;
	}

	inline int32_t GetFillRate() const
	{
		const float max_threshold = 0.875f;
		const float target_count = GetTargetCount();
		if (target_count > 0.0f)
		{
			float percent = fminf(targets_hit / target_count / max_threshold, 1.0f);
			return static_cast<int32_t>(percent * 15);
		}

		return 0;
	}

	inline bool CheckTargetInRange(int32_t index) const
	{
		return index >= first_target_index && index <= last_target_index;
	}
};

struct StateEx
{
	std::list<TargetStateEx*> target_references;
	FileHandler file_handler;
	int32_t file_state;
	bool dsc_loaded;
	bool files_loaded;
	std::vector<TargetStateEx> target_ex;
	ChanceState chance_time;

	inline void ResetPlayState()
	{
		target_references.clear();
		for (TargetStateEx& ex : target_ex)
			ex.ResetPlayState();
		chance_time.ResetPlayState();
	}

	inline bool PushTarget(TargetStateEx* ex)
	{
		for (TargetStateEx* p : target_references)
			if (p == ex)
				return false;

		target_references.push_back(ex);
		return true;
	}

	inline bool PopTarget(TargetStateEx* ex)
	{
		for (auto it = target_references.begin(); it != target_references.end(); it++)
		{
			if (*it == ex)
			{
				target_references.erase(it);
				return true;
			}
		}

		return false;
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

static inline bool IsStarLikeNote(int32_t type)
{
	return type == TargetType_Star ||
		type == TargetType_ChanceStar ||
		type == TargetType_LinkStar ||
		type == TargetType_LinkStarEnd;
}

static inline bool IsLinkStarNote(int32_t type, bool exclude_end)
{
	return type == TargetType_LinkStar || (type == TargetType_LinkStarEnd && !exclude_end);
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