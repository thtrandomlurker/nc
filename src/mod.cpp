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
		ex.long_end = end;
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

HOOK(int32_t, __fastcall, ParseTargets, 0x140245C50, PVGameData* pv_game)
{
	// TODO: Rewrite this function to accomodate our custom note's scoring
	//
	int32_t ret = originalParseTargets(pv_game);

	// NOTE: Initialize extra data for targets
	int32_t index = 0;
	for (PvDscTargetGroup& group : pv_game->pv_data.targets)
	{
		for (int i = 0; i < group.target_count; i++)
		{
			// NOTE: Patch existing extra data with new information
			//
			if (TargetStateEx* ex = GetTargetStateEx(index, i); ex != nullptr)
			{
				ex->target_type = group.targets[i].type;
				continue;
			}

			// NOTE: Append new extra data to the list
			//
			TargetStateEx ex = { };
			ex.target_index = index;
			ex.sub_index = i;
			ex.target_type = group.targets[i].type;
			ex.ResetPlayState();
			state.target_ex.push_back(ex);
		}

		index++;
	}

	// NOTE: Resolve target relations
	//
	auto findNextTarget = [&pv_game](size_t start_index, int32_t start_sub, int32_t type, int32_t type2, bool end)
	{
		for (size_t i = start_index; i < pv_game->pv_data.targets.size(); i++)
		{
			PvDscTargetGroup* group = &pv_game->pv_data.targets[i];
			for (int sub = start_sub; sub < group->target_count; sub++)
			{
				TargetStateEx* ex = GetTargetStateEx(i, sub);
				if (group->targets[sub].type == type || group->targets[sub].type == type2 || type == -1)
				{
					if (ex->long_end == end)
						return std::pair(&group->targets[sub], ex);
				}
			}
		}

		return std::pair<PvDscTarget*, TargetStateEx*>(nullptr, nullptr);
	};

	TargetStateEx* previous = nullptr;
	for (size_t i = 0; i < pv_game->pv_data.targets.size(); i++)
	{
		PvDscTargetGroup* group = &pv_game->pv_data.targets[i];
		for (int sub = 0; sub < group->target_count; sub++)
		{
			PvDscTarget* tgt = &group->targets[sub];
			TargetStateEx* ex = GetTargetStateEx(i, sub);

			// NOTE: Link long note pieces together
			//
			if (IsLongNote(tgt->type) && !ex->long_end)
			{
				TargetStateEx* next = findNextTarget(i + 1, sub, tgt->type, -1, true).second;
				if (next != nullptr)
				{
					ex->next = next;
					next->prev = ex;
				}
			}
			// NOTE: Resolve LinkStars
			else if (tgt->type == TargetType_LinkStar || tgt->type == TargetType_LinkStarEnd)
			{
				if (tgt->type == TargetType_LinkStar)
				{
					if (ex->prev == nullptr)
						ex->link_start = true;

					TargetStateEx* next = findNextTarget(
						i + 1, 
						sub,
						TargetType_LinkStar,
						TargetType_LinkStarEnd,
						false
					).second;

					if (next != nullptr)
					{
						ex->next = next;
						next->prev = ex;
					}

					ex->link_step = true;
				}
				else if (tgt->type == TargetType_LinkStarEnd)
				{
					ex->link_step = true;
					ex->link_end = true;
				}
			}
		}
	}

	// NOTE: Find chance time
	//
	int32_t pos = 1;
	int32_t time = -1;
	int32_t last_time = -1;
	int64_t chance_start_time = -1;
	int64_t chance_end_time = -1;
	while (true)
	{
		int32_t branch = 0;
		pos = FindNextCommand(&pv_game->pv_data, 26, &time, &branch, pos);

		if (pos != -1)
		{
			if (time != -1)
				last_time = time;

			// TODO: Add difficulty check
			if (pv_game->pv_data.script_buffer[pos + 2] == 4)
				chance_start_time = static_cast<int64_t>(last_time) * 10000;
			else if (pv_game->pv_data.script_buffer[pos + 2] == 5)
				chance_end_time = static_cast<int64_t>(last_time) * 10000;

			/*
			printf("%d %d %d (%d %d %d)\n", pos, time, branch, pv_game->pv_data.script_buffer[pos], pv_game->pv_data.script_buffer[pos + 1], pv_game->pv_data.script_buffer[pos + 2]);
			*/

			pos += diva::dsc::GetOpcodeInfo(26)->length + 1;
			continue;
		}

		break;
	}

	// NOTE: Figure out which notes are part of the chance time
	//
	state.chance_time.first_target_index = -1;
	state.chance_time.last_target_index = -1;
	if (chance_start_time != -1 && chance_end_time != -1)
	{
		for (size_t i = 0; i < pv_game->pv_data.targets.size(); i++)
		{
			PvDscTargetGroup* tgt = &pv_game->pv_data.targets[i];
			if (tgt->hit_time >= chance_start_time && tgt->hit_time <= chance_end_time)
			{
				if (state.chance_time.first_target_index == -1)
					state.chance_time.first_target_index = i;
				state.chance_time.last_target_index = i;
			}
		}
	}

	for (TargetStateEx& ex : state.target_ex)
	{
		if (ex.next == nullptr && ex.prev == nullptr)
			continue;

		printf("TARGET:  %d/%d  %d  %d:%.3f  <%d-%d-%d>  ", ex.target_index, ex.sub_index, ex.target_type, ex.long_end, ex.length, ex.link_start, ex.link_step, ex.link_end);
		if (ex.next != nullptr)
			printf("[NEXT:%d/%d]  ", ex.next->target_index, ex.next->sub_index);

		if (ex.prev != nullptr)
			printf("[PREV:%d/%d]  ", ex.prev->target_index, ex.prev->sub_index);

		printf("\n");
	}

	return ret;
}

extern "C"
{
	void __declspec(dllexport) Init()
	{
		freopen("CONOUT$", "w", stdout);

		// NOTE: Patch target type check in PVGameTarget::CreateAet (0x150D54750)
		WRITE_MEMORY(0x150D54766, uint8_t, TargetType_Max - 12);

		// NOTE: Install hooks
		INSTALL_HOOK(TaskPvGameInit);
		INSTALL_HOOK(TaskPvGameCtrl);
		INSTALL_HOOK(TaskPvGameDest);
		INSTALL_HOOK(ParseTargets);
		INSTALL_HOOK(LoadDscCtrl);
		InstallHitStateHooks();
		InstallTargetHooks();
	}
};