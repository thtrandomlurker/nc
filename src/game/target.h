#pragma once

#include <stdint.h>

struct NoteSprite
{
	const char* ft;  // AC / AFT / PlayStation
	const char* nsw; // Nintendo Switch
	const char* pc;  // Steam / Xbox
};

const char* GetTargetLayer(int32_t target_type);
const char* GetButtonLayer(int32_t target_type);

namespace nc
{

}

void InstallTargetHooks();