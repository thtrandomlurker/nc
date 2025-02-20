#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "Helpers.h"
#include "common.h"
#include "megamix.h"
#include "game/hit_state.h"
#include "game/target.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static bool ParseExtraCSV(const char* data, size_t size)
{
	state.target_ex.clear();

	if (data == nullptr || size == 0)
		return false;

	std::vector<std::string_view> lines;
	const char* head = data;
	const char* line = head;
	size_t position = 0;
	while (true)
	{
		// NOTE: Parse lines
		bool comment = (*line == ';');

		if (*head == '\n')
		{
			if (!comment)
				lines.push_back(std::string_view(line, head - line));
			line = head + 1;
		}

		head++;
		position++;
		if (position >= size)
		{
			if (line != head && !comment)
				lines.push_back(std::string_view(line, head - line));
			break;
		}
	}

	// NOTE: Parse lines
	for (std::string_view line : lines)
	{
		int32_t target_index = 0;
		float length = 0.0f;
		int32_t end = 0;
		_snscanf(line.data(), line.size(), "%d,%f,%d", &target_index, &length, &end);

		TargetStateEx& ex = state.target_ex.emplace_back();
		ex.target_index = target_index;
		ex.length = length / 1000.0f;
		ex.end = end;
		ex.ResetPlayState();
	}

	return true;
}

// NOTE: Hook TaskPvGame routines to load our own assets
//
HOOK(bool, __fastcall, TaskPvGameInit, 0x1405DA040, uint64_t a1)
{
	state.files_loaded = false;
	state.dsc_loaded = false;
	state.file_state = 0;

	prj::string str;
	prj::string_view strv;
	diva::aet::LoadAetSet(AetSetID, &str);
	diva::spr::LoadSprSet(SprSetID, &strv);

	state.ResetPlayState();
	return originalTaskPvGameInit(a1);
}

HOOK(bool, __fastcall, TaskPvGameCtrl, 0x1405DA060, uint64_t a1)
{
	if (!state.files_loaded)
	{
		state.files_loaded = !diva::aet::CheckAetSetLoading(AetSetID) &&
			!diva::spr::CheckSprSetLoading(SprSetID);
	}

	return originalTaskPvGameCtrl(a1);
}

HOOK(bool, __fastcall, TaskPvGameDest, 0x1405DA0A0, uint64_t a1)
{
	if (state.files_loaded)
	{
		diva::aet::UnloadAetSet(AetSetID);
		diva::spr::UnloadSprSet(SprSetID);
		state.files_loaded = false;
	}

	return originalTaskPvGameDest(a1);
}

// NOTE: Hook LoadDscCtrl to handle loading our external CSV file
//
HOOK(bool, __fastcall, LoadDscCtrl, 0x14024E270, PVGamePvData* pv_data, prj::string* path, void* a3, bool a4)
{
	if (state.file_state == 0)
	{
		// NOTE: Get filename without extension
		const char* point = strchr(path->c_str(), '.');
		size_t length = point != nullptr ? point - path->c_str() : path->size();

		// NOTE: Format file name
		char csv_path[256] = { '\0' };
		sprintf_s(csv_path, "%.*s_nc.csv", static_cast<uint32_t>(length), path->c_str());

		// NOTE: Check if file exists and start reading it
		prj::string csv_path_str = csv_path;
		prj::string fixed;
		if (FileCheckExists(&csv_path_str, &fixed))
		{
			printf("File (%s) exists! Found at (%s)\n", csv_path_str.c_str(), fixed.c_str());
			if (!FileRequestLoad(&state.file_handler, csv_path_str.c_str(), 1))
			{
				state.file_handler = nullptr;
				state.file_state = 2;
			}
			else
				state.file_state = 1;
		}
		else
		{
			printf("File (%s) does not exist!\n", csv_path_str.c_str());
			state.file_handler = nullptr;
			state.file_state = 2;
		}
	}
	else if (state.file_state == 1)
	{
		if (!FileCheckNotReady(&state.file_handler))
			state.file_state = 2;
	}
	else if (state.file_state == 2 && state.file_handler != nullptr)
	{
		const void* data = FileGetData(&state.file_handler);
		size_t size = FileGetSize(&state.file_handler);
		ParseExtraCSV(static_cast<const char*>(data), size);
		FileFree(&state.file_handler);
	}

	if (!state.dsc_loaded)
	{
		state.dsc_loaded = originalLoadDscCtrl(pv_data, path, a3, a4);
		return false;
	}

	return state.file_state == 2;
}

// NOTE: Important functions
//
HOOK(void, __fastcall, PvGameTarget_CreateAetLayers, 0x14026F910, PvGameTarget* target)
{
	if (target->target_type < TargetType_Custom || target->target_type >= TargetType_Max)
		return originalPvGameTarget_CreateAetLayers(target);

	// NOTE: Remove previously created aet objects, if present
	diva::aet::Stop(&target->target_aet);
	diva::aet::Stop(&target->button_aet);
	diva::aet::Stop(&target->target_eff_aet);
	diva::aet::Stop(&target->dword78);

	const char* target_layer = GetTargetLayer(target->target_type);
	const char* button_layer = GetButtonLayer(target->target_type);

	diva::vec2 target_pos;
	diva::vec2 button_pos;
	diva::GetScaledPosition(&target->target_pos, &target_pos);
	diva::GetScaledPosition(&target->button_pos, &button_pos);

	target->target_aet = diva::aet::PlayLayer(
		AetSceneID,
		8,
		0x20000,
		target_layer,
		&target_pos,
		0,
		nullptr,
		nullptr,
		0.0f,
		-1.0f,
		0,
		nullptr
	);

	target->button_aet = diva::aet::PlayLayer(
		AetSceneID,
		9,
		0x20000,
		button_layer,
		&button_pos,
		0,
		nullptr,
		nullptr,
		0.0f,
		-1.0f,
		0,
		nullptr
	);

	// NOTE: Initialize some information
	target->out_start_time = 360.0f;
	target->scaling_end_time = 31.0f;
}

/*
HOOK(uint32_t, __fastcall, PVGameArcade_DetermineTarget, 0x14026E8E0, void* a1, PvDscTarget* a2, float a3, int32_t a4, int32_t a5, int32_t a6, int64_t a7, int64_t a8, int32_t a9)
{
	printf("[DetermineTarget] %f %d %d %d %lld %lld %d\n", a3, a4, a5, a6, a7, a8, a9);
	printf("  (%.2f, %.2f)  (%.2f, %.2f)  %d %.3f %d %d %d %d %d %d\n", a2->target_pos.x, a2->target_pos.y, a2->start_pos.x, a2->start_pos.y, a2->type, a2->amplitude, a2->frequency, a2->slide_chain_start, a2->slide_chain_end, a2->slide_chain_left, a2->slide_chain_right, a2->slide_chain);

	return originalPVGameArcade_DetermineTarget(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

HOOK(uint64_t, __fastcall, PVGameArcade_UpdateTargets, 0x14026DD80, PVGameArcade* data, float dt)
{
	return originalPVGameArcade_UpdateTargets(data, dt);
}

HOOK(uint64_t, __fastcall, PVGameArcade_InitKiseki, 0x150D50360, PVGameArcade* data, PvGameTarget* target)
{
	uint64_t ret = originalPVGameArcade_InitKiseki(data, target);
	if (data->b1)
		target->kiseki_width = 2.0;

	return ret;
}
*/

extern "C"
{
	void __declspec(dllexport) Init()
	{
		/*
		INSTALL_HOOK(PVGameArcade_UpdateTargets);
		INSTALL_HOOK(PVGameArcade_DetermineTarget);
		INSTALL_HOOK(PVGameArcade_InitKiseki);
		*/

		freopen("CONOUT$", "w", stdout);

		// NOTE: Patch target type check in PVGameTarget::CreateAet (0x150D54750)
		WRITE_MEMORY(0x150D54766, uint8_t, TargetType_Max - 12);

		// NOTE: Install hooks
		INSTALL_HOOK(TaskPvGameInit);
		INSTALL_HOOK(TaskPvGameCtrl);
		INSTALL_HOOK(TaskPvGameDest);
		INSTALL_HOOK(LoadDscCtrl);
		INSTALL_HOOK(PvGameTarget_CreateAetLayers);
		InstallHitStateHooks();
		InstallTargetHooks();
	}
};