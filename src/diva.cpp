#include <string.h>
#include "diva.h"

FUNCTION_PTR(AetArgs*, __fastcall, CreateAetArgsOrg, 0x14028D560, AetArgs*, uint32_t, const char*, int32_t, int32_t);
FUNCTION_PTR(void, __fastcall, StopAetOrg, 0x1402CA330, int32_t* id);
FUNCTION_PTR(SprArgs*, __fastcall, DefaultSprArgs, 0x1405B78D0, SprArgs* args);
FUNCTION_PTR(TextArgs*, __fastcall, DefaultTextArgs, 0x1402C53D0, TextArgs* args);
FUNCTION_PTR(AetArgs*, __fastcall, DefaultAetArgs, 0x1401A87F0, AetArgs* args);

static FUNCTION_PTR(int32_t, __fastcall, PlayLayerImp, 0x14027B420, uint32_t scene_id, int32_t prio, int32_t flags, const char* layer, const diva::vec2* pos, int32_t index, const char* start_marker, const char* end_marker, float start_time, float end_time, int32_t a11, void* frmctl);

static FUNCTION_PTR(void*, __fastcall, CreateAetArgsAction, 0x14028D560, AetArgs* args, uint32_t scene_id, const char* layer, int32_t prio, int32_t action);
static FUNCTION_PTR(int32_t, __fastcall, PlayLayerAetArgs, 0x1402CA220, AetArgs* args, int32_t id);

AetArgs::AetArgs(uint32_t scene, const char* layer, int32_t prio, int32_t marker_mode)
{
	CreateAetArgsAction(this, scene, layer, prio, marker_mode);
}

SprArgs::SprArgs()
{
	memset(this, 0, sizeof(SprArgs));
	DefaultSprArgs(this);
}
	
TextArgs::TextArgs()
{
	memset(this, 0, sizeof(TextArgs));
	DefaultTextArgs(this);
}

/*
AetArgs::AetArgs()
{
	memset(this, 0, sizeof(AetArgs));
	DefaultAetArgs(this);
}
*/

// NOTE: Temporary. Just until I sort out the std::map types.
/*
FUNCTION_PTR(void, __fastcall, DestroyAetArgs, 0x1401A9100, AetArgs* args);
AetArgs::~AetArgs()
{
	DestroyAetArgs(this);
}
*/

void aet::CreateAetArgs(AetArgs* args, uint32_t scene_id, const char* layer_name, int32_t prio)
{
	CreateAetArgsOrg(args, scene_id, layer_name, prio, 0);
}

void aet::CreateAetArgs(AetArgs* args, uint32_t scene_id, const char* layer_name, int32_t flags, int32_t layer, int32_t prio, const char* start_marker, const char* end_marker)
{
	CreateAetArgs(args, scene_id, layer_name, prio);
	args->flags = flags;
	args->layer = layer;
	args->start_marker = start_marker;
	args->end_marker = end_marker;
}

void aet::Stop(int32_t id)
{
	if (id != 0)
		StopAetOrg(&id);
}

void aet::Stop(int32_t* id)
{
	if (id != nullptr)
	{
		if (*id != 0)
			StopAetOrg(id);
		*id = 0;
	}
}

bool aet::StopOnEnded(int32_t* id)
{
	if (id != nullptr && *id != 0)
	{
		bool ended = aet::GetEnded(*id);
		if (ended)
			aet::Stop(id);
		return ended;
	}

	return true;
}

int32_t aet::PlayLayer(uint32_t scene_id, int32_t prio, int32_t flags, const char* layer, const diva::vec2* pos, int32_t index, const char* start_marker, const char* end_marker, float start_time, float end_time, int32_t a11, void* frmctl)
{
	return PlayLayerImp(
		scene_id,
		prio,
		flags,
		layer,
		pos,
		index,
		start_marker,
		end_marker,
		start_time,
		end_time,
		a11,
		frmctl
	);
}

int32_t aet::PlayLayer(uint32_t scene, int32_t prio, int32_t flags, const char* layer, const diva::vec2* pos, const char* start_marker, const char* end_marker)
{
	return PlayLayer(scene, prio, flags, layer, pos, 0, start_marker, end_marker, -1.0f, -1.0f, 0, nullptr);
}

int32_t aet::PlayLayer(uint32_t scene_id, int32_t prio, const char* layer, int32_t action)
{
	AetArgs args;
	CreateAetArgsAction(&args, scene_id, layer, prio, action);
	return PlayLayerAetArgs(&args, 0);
}

// NOTE: InputState implementation
static FUNCTION_PTR(float, __fastcall, IS_GetPosition, 0x1402AB2C0, diva::InputState* t, int32_t index);
static FUNCTION_PTR(int32_t, __fastcall, IS_GetDeviceVendor, 0x1402AAF20, diva::InputState* t);
static FUNCTION_PTR(bool, __fastcall, IS_IsButtonTapped, 0x1402AB260, diva::InputState* t, int32_t key);
static FUNCTION_PTR(bool, __fastcall, IS_IsButtonTappedAbs, 0x1402AB160, diva::InputState* t, int32_t key);
static FUNCTION_PTR(bool, __fastcall, IS_IsButtonRepeat, 0x1402AB270, diva::InputState* t, int32_t key);
static FUNCTION_PTR(bool, __fastcall, IS_IsButtonDown, 0x1402AB280, diva::InputState* t, int32_t key);
static FUNCTION_PTR(int32_t, __fastcall, GetNormalizedDeviceType, 0x1402ACAA0, int32_t vendor);

float diva::InputState::GetPosition(int32_t index) { return IS_GetPosition(this, index); }
int32_t diva::InputState::GetDevice() { return GetNormalizedDeviceType(IS_GetDeviceVendor(this)); }
bool diva::InputState::IsButtonTapped(int32_t key) { return IS_IsButtonTapped(this, key); }
bool diva::InputState::IsButtonTappedAbs(int32_t key) { return CheckButtonDelegate(this, key, IS_IsButtonTappedAbs); }
bool diva::InputState::IsButtonDown(int32_t key) { return IS_IsButtonDown(this, key); }
bool diva::InputState::IsButtonTappedOrRepeat(int32_t key) { return IS_IsButtonRepeat(this, key); }

// NOTE: PVGameArcade implementation
static FUNCTION_PTR(void, __fastcall, PVGAC_EraseTarget, 0x14026E5C0, PVGameArcade* data, PvGameTarget* target);
static FUNCTION_PTR(void, __fastcall, PVGAC_FinishTargetAet, 0x14026E640, PVGameArcade* data, PvGameTarget* target);
static FUNCTION_PTR(void, __fastcall, PVGAC_PlayHitEffect, 0x1402715F0, PVGameArcade* data, int32_t effect, const diva::vec2* pos);

void PVGameArcade::EraseTarget(PvGameTarget* target) { PVGAC_EraseTarget(this, target); }
void PVGameArcade::RemoveTargetAet(PvGameTarget* target) { PVGAC_FinishTargetAet(this, target); }
void PVGameArcade::PlayHitEffect(int32_t index, const diva::vec2& pos) { PVGAC_PlayHitEffect(this, index, &pos); }

// NOTE:  PVGameUI implementation
static FUNCTION_PTR(void, __fastcall, PGUI_SetBonusText, 0x140275670, PVGameUI*, int32_t, float, float);

void PVGameUI::SetBonusText(int32_t value, float x, float y) { PGUI_SetBonusText(this, value, x, y); }

// NOTE: Misc
static FUNCTION_PTR(void, __fastcall, DSC_ScalePosition, 0x1402666E0, const diva::vec2* in, diva::vec2* out);

diva::vec2 GetScaledPosition(const diva::vec2& v)
{
	diva::vec2 scaled = { };
	DSC_ScalePosition(&v, &scaled);
	return scaled;
}