#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include <d3d11.h>
#include <detours.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <thirdparty/lazycsv.hpp>
#include "hooks.h"
#include "game/hit_state.h"
#include "game/target.h"
#include "game/chance_time.h"
#include "nc_log.h"
#include "game/game.h"
#include "ui/pv_sel.h"
#include "ui/customize_sel.h"
#include "db.h"
#include "save_data.h"
#include "util.h"
#include <thirdparty/toml.h>
#include <thirdparty/imgui/imgui_impl_dx11.h>
#include <thirdparty/imgui/imgui_impl_win32.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const int32_t* life_table = reinterpret_cast<const int32_t*>(0x140BE9EA0);

static bool ParseExtraCSV(const void* data, size_t size)
{
	state.target_ex.clear();

	if (data == nullptr || size == 0)
		return false;

	std::string_view view = std::string_view(static_cast<const char*>(data), size);
	lazycsv::parser<std::string_view> parser { view };

	auto header = parser.header();
	for (const auto row : parser)
	{
		int32_t index = -1;
		int32_t sub_index = 0;
		float length = -1.0f;
		bool is_end = false;

		int32_t cell_index = 0;
		for (const auto cell : row)
		{
			std::string_view name = header.cells(cell_index)[0].trimed();
			std::string value(cell.trimed()); // NOTE: Should usually be small enough that C++ does SSO

			if (name == "index")
				index = std::stoi(value);
			else if (name == "sub_index")
				sub_index = std::stoi(value);
			else if (name == "length")
				length = std::stof(value);
			else if (name == "end")
			{
				if (value == "true")
					is_end = true;
				else if (value == "false")
					is_end = false;
				else
				{
					int32_t i = std::stoi(value);
					is_end = i > 0;
				}
			}

			cell_index++;
		}

		if (index != -1)
		{
			TargetStateEx& ex = state.target_ex.emplace_back();
			ex.target_index = index;
			ex.sub_index = sub_index;
			ex.length = length > 0.0f ? length / 1000.0f : -1.0f;
			ex.long_end = is_end;
			ex.ResetPlayState();
		}
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
	aet::LoadAetSet(AetSetID, &str);
	spr::LoadSprSet(SprSetID, &strv);
	if (!sound::RequestFarcLoad("rom/sound/se_nc.farc"))
		nc::Print("Failed to load se_nc.farc\n");

	state.Reset();
	return originalTaskPvGameInit(a1);
}

HOOK(bool, __fastcall, TaskPvGameCtrl, 0x1405DA060, uint64_t a1)
{
	if (!state.files_loaded)
	{
		state.files_loaded = !aet::CheckAetSetLoading(AetSetID) &&
			!spr::CheckSprSetLoading(SprSetID) &&
			!sound::IsFarcLoading("rom/sound/se_nc.farc");
	}

	return originalTaskPvGameCtrl(a1);
}

HOOK(bool, __fastcall, TaskPvGameDest, 0x1405DA0A0, uint64_t a1)
{
	state.ResetPlayState();
	state.ui.ResetAllLayers();

	if (state.files_loaded)
	{
		aet::UnloadAetSet(AetSetID);
		spr::UnloadSprSet(SprSetID);
		sound::UnloadFarc("rom/sound/se_nc.farc");
		state.files_loaded = false;
	}

	return originalTaskPvGameDest(a1);
}

HOOK(void, __fastcall, PVGameReset, 0x1402436F0, void* pv_game)
{
	nc::Print("PVGameReset()\n");

	state.ResetPlayState();
	state.ui.ResetAllLayers();
	originalPVGameReset(pv_game);
}

HOOK(void, __fastcall, PVGameArcadeReset, 0x14026AE80, PVGameArcade* game)
{
	state.ResetAetData();
	originalPVGameArcadeReset(game);
}

// NOTE: Hook LoadDscCtrl to handle loading our external CSV file
//
HOOK(bool, __fastcall, LoadDscCtrl, 0x14024E270, PVGamePvData* pv_data, prj::string* path, void* a3, bool a4)
{
	prj::string dsc_file_path = *path;
	prj::string csv_file_path = "";

	if (state.nc_chart_entry.has_value())
	{
		const std::string& script_file_name = state.nc_chart_entry.value().script_file_name;
		if (!script_file_name.empty() && script_file_name != "(NULL)")
		{
			dsc_file_path = script_file_name;
			csv_file_path = util::ChangeExtension(script_file_name, ".csv");
		}
	}

	switch (state.file_state)
	{
	case 0:
		if (csv_file_path.empty())
		{
			state.file_state = 3;
			break;
		}

		if (!FileRequestLoad(&state.file_handler, csv_file_path.c_str(), 1))
		{
			state.file_handler = nullptr;
			state.file_state = 3;
			break;
		}

		nc::Print("File (%s) exists!\n", csv_file_path.c_str());
		state.file_state = 1;
		break;
	case 1:
		if (!FileCheckNotReady(&state.file_handler))
			state.file_state = 2;
		break;
	case 2:
		ParseExtraCSV(FileGetData(&state.file_handler), FileGetSize(&state.file_handler));
		FileFree(&state.file_handler);
		state.file_handler = nullptr;
		state.file_state = 3;
		break;
	case 3:
		break;
	}

	if (!state.dsc_loaded)
		state.dsc_loaded = originalLoadDscCtrl(pv_data, &dsc_file_path, a3, a4);

	return state.dsc_loaded && state.file_state == 3;
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
			if (ex->IsLongNoteStart())
			{
				TargetStateEx* next = findNextTarget(i + 1, sub, tgt->type, -1, true).second;
				if (next != nullptr)
				{
					ex->next = next;
					next->prev = ex;

					// NOTE: Auto-calculate long note length if necessary
					if (ex->length < 0.0f)
					{
						PvDscTargetGroup* next_group = &pv_game->pv_data.targets[next->target_index];
						ex->length = static_cast<float>(next_group->hit_time - group->hit_time) / 1000000000.0f;
					}
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
			// NOTE: Resolve rush note info
			else if (ex->IsRushNote())
				ex->bal_max_hit_count = ex->length * 4.5f;
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

			pos += dsc::GetOpcodeInfo(26)->length + 1;
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

	// NOTE: Calculate percentage parameters for F2nd score mode
	if (state.GetScoreMode() == ScoreMode_F2nd)
	{
		float targets_max_rate = 1.0f;
		if (state.chance_time.IsValid())
			targets_max_rate -= ChanceTimeRetainedRate;

		// NOTE: Patch target reference scores, to make the percentages borders
		//       move more accurately to the console gameplay flow.
		float target_max_score = pv_game->reference_score * targets_max_rate;
		float target_step_score = target_max_score / pv_game->pv_data.targets.size();
		float ct_retained_score = pv_game->reference_score * ChanceTimeRetainedRate;
		float cur_ref_score = 0.0f;
		
		pv_game->target_reference_scores.clear();
		pv_game->target_reference_scores.push_back(0);

		int32_t tgt_index = 0;
		for (const auto& group : pv_game->pv_data.targets)
		{
			cur_ref_score += target_step_score;
			if (state.chance_time.IsValid() && tgt_index == state.chance_time.last_target_index)
				cur_ref_score += ct_retained_score;

			pv_game->target_reference_scores.push_back(cur_ref_score);
			tgt_index++;
		}

		state.scoring_info.target_max_rate = targets_max_rate;
	}

	// NOTE: Patch score reference (Only in Arcade mode; We don't need to patch this in F2nd mode as
	//                              we use our own percentage calculation algorithm)
	if (state.GetScoreMode() == ScoreMode_Arcade)
	{
		int32_t life = 127;
		int32_t total_chance_bonus = 0;
		int32_t total_hold_bonus = 0;
		int32_t total_link_bonus = 0;

		for (size_t i = 0; i < pv_game->pv_data.targets.size(); i++)
		{
			for (int j = 0; j < pv_game->pv_data.targets[i].target_count; j++)
			{
				PvDscTarget& tgt = pv_game->pv_data.targets[i].targets[j];
				TargetStateEx* ex = GetTargetStateEx(i, j);

				if (ex->IsLinkNote())
					total_link_bonus += 200;

				if (state.chance_time.CheckTargetInRange(i))
				{
					total_chance_bonus += 1000;

					// NOTE: The game will apply life bonus to notes in chance time because
					//       it isn't aware of chance times, so we need to deduct those too.
					if (life == 255)
						pv_game->reference_score_with_life -= 10;
				}

				pv_game->target_reference_scores[i + 1] += total_chance_bonus + total_link_bonus;
			}

			if (!state.chance_time.CheckTargetInRange(i))
			{
				life += life_table[21 * GetPvGameplayInfo()->difficulty];
				if (life > 255)
					life = 255;
			}
		}

		pv_game->reference_score += total_chance_bonus + total_hold_bonus + total_link_bonus;
		pv_game->reference_score_with_life += total_chance_bonus + total_hold_bonus + total_link_bonus;
	}

	for (TargetStateEx& ex : state.target_ex)
	{
		if (ex.next == nullptr && ex.prev == nullptr)
			continue;

		nc::Print("TARGET %03d/%03d:  %02d  %d:%.3f  <%d-%d-%d>\n", ex.target_index, ex.sub_index, ex.target_type, ex.long_end, ex.length, ex.link_start, ex.link_step, ex.link_end);
	}

	return pv_game->reference_score;
}

static void LoadConfig()
{
	auto res = toml::parse_file("config.toml");
	if (!res.succeeded())
		return;

	float stick_sensivity = fmaxf(fminf(res.table()["controller"]["stick_sensitivity"].value_or(50.0f), 100.0f), 0.0f);
	macro_state.sensivity = 1.0f - stick_sensivity / 100.0f;
}

extern "C"
{
	static ID3D11DeviceContext* ctx = nullptr;
	static ID3D11RenderTargetView* main_render_target = nullptr;
	static WNDPROC wnd_proc = nullptr;
	static std::string path = ".\\";

	void __declspec(dllexport) Init()
	{
		freopen("CONOUT$", "w", stdout);

		LoadConfig();

		// NOTE: Patch target type check in PVGameTarget::CreateAet (0x150D54750)
		WRITE_MEMORY(0x150D54766, uint8_t, TargetType_Max - 12);

		// NOTE: Install hooks
		INSTALL_HOOK(TaskPvGameInit);
		INSTALL_HOOK(TaskPvGameCtrl);
		INSTALL_HOOK(TaskPvGameDest);
		INSTALL_HOOK(PVGameReset);
		INSTALL_HOOK(PVGameArcadeReset);
		INSTALL_HOOK(ParseTargets);
		INSTALL_HOOK(LoadDscCtrl);
		InstallGameHooks();
		InstallTargetHooks();
		InstallDatabaseHooks();
		nc::CreateDefaultSaveData();
		nc::InstallSaveDataHooks();

		// NOTE: Get the full path of this DLL file
		HMODULE handle = NULL;
		GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)Init,
			&handle
		);

		TCHAR buffer[MAX_PATH] = { 0 };
		GetModuleFileName(handle, buffer, MAX_PATH);
		PathRemoveFileSpecW(buffer);

		char narrow_buffer[512] = { 0 };
		::WideCharToMultiByte(CP_UTF8, 0, buffer, -1, narrow_buffer, 512, nullptr, nullptr);
		path = narrow_buffer;
	}

	void __declspec(dllexport) PostInit()
	{
		InstallPvSelHooks();
		InstallCustomizeSelHooks();
	}

	LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
			return true;
		if (ImGui::GetCurrentContext() != 0 && ImGui::GetIO().WantCaptureMouse) {
			switch (uMsg) {
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_RBUTTONUP:
			case WM_MOUSEWHEEL:
				return true;
			}
		}

		return CallWindowProc(wnd_proc, hWnd, uMsg, wParam, lParam);
	}

	void __declspec(dllexport) D3DInit(IDXGISwapChain* sc, ID3D11Device* dev, ID3D11DeviceContext* ctx)
	{
		DXGI_SWAP_CHAIN_DESC scd = { };
		sc->GetDesc(&scd);

		// NOTE: Retrieve render texture and create render target
		ID3D11Texture2D* back_buffer = nullptr;
		sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
		if (back_buffer != nullptr)
		{
			dev->CreateRenderTargetView(back_buffer, nullptr, &main_render_target);
			back_buffer->Release();
		}

		// NOTE: Create WndProc
		wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
			scd.OutputWindow,
			GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(WndProc)
		));
		
		// NOTE: Create our ImGui context
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		// NOTE: Load fonts
		io.Fonts->AddFontDefault();

		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphOffset.y = 3.5f;
		static const ImWchar icon_ranges[] = { 0xE000, 0xE0FF };
		io.Fonts->AddFontFromFileTTF((::path + "\\ram\\OpenFontIcons.ttf").c_str(), 13.0f, &config, icon_ranges);

		ImGui_ImplWin32_Init(scd.OutputWindow);
		ImGui_ImplDX11_Init(dev, ctx);

		::ctx = ctx;
	}

	void __declspec(dllexport) OnFrame(IDXGISwapChain* sc)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		customize_sel::ShowWindow();

		ImGui::Render();
		ctx->OMSetRenderTargets(1, &main_render_target, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
};