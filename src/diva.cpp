#include <string.h>
#include "diva.h"
#include "Helpers.h"

namespace prj
{
	FUNCTION_PTR(void*, __fastcall, operatorNew, 0x1409777D0, size_t);
	FUNCTION_PTR(void*, __fastcall, operatorDelete, 0x1409B1E90, void*);
}

namespace diva
{
	FUNCTION_PTR(AetArgs*, __fastcall, CreateAetArgsOrg, 0x14028D560, AetArgs*, uint32_t, const char*, int32_t, int32_t);
	FUNCTION_PTR(void, __fastcall, StopAetOrg, 0x1402CA330, int32_t* id);
	FUNCTION_PTR(SprArgs*, __fastcall, DefaultSprArgs, 0x1405B78D0, SprArgs* args);
	FUNCTION_PTR(TextArgs*, __fastcall, DefaultTextArgs, 0x1402C53D0, TextArgs* args);
	FUNCTION_PTR(AetArgs*, __fastcall, DefaultAetArgs, 0x1401A87F0, AetArgs* args);

	SprArgs::SprArgs()
	{
		memset(this, 0, sizeof(diva::SprArgs));
		DefaultSprArgs(this);
	}
	
	TextArgs::TextArgs()
	{
		memset(this, 0, sizeof(diva::TextArgs));
		DefaultTextArgs(this);
	}

	AetArgs::AetArgs()
	{
		memset(this, 0, sizeof(diva::AetArgs));
		DefaultAetArgs(this);
	}

	// NOTE: Temporary. Just until I sort out the std::map types.
	FUNCTION_PTR(void, __fastcall, DestroyAetArgs, 0x1401A9100, AetArgs* args);
	AetArgs::~AetArgs()
	{
		DestroyAetArgs(this);
	}

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

	// NOTE: InputState implementation
	static FUNCTION_PTR(float, __fastcall, IS_GetPosition, 0x1402AB2C0, InputState* t, int32_t index);
	static FUNCTION_PTR(int32_t, __fastcall, IS_GetDeviceVendor, 0x1402AAF20, InputState* t);
	static FUNCTION_PTR(int32_t, __fastcall, GetNormalizedDeviceType, 0x1402ACAA0, int32_t vendor);

	float InputState::GetPosition(int32_t index) { return IS_GetPosition(this, index); }
	int32_t InputState::GetDevice() { return GetNormalizedDeviceType(IS_GetDeviceVendor(this)); }
}