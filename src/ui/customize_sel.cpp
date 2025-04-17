#include <stdint.h>
#include <vector>
#include <diva.h>
#include <hooks.h>
#include <nc_log.h>
#include <sound_db.h>
#include <save_data.h>
#include <thirdparty/imgui/imgui.h>
#include "customize_sel.h"

// NOTE: Helper functions
template <typename T>
const T* FindWithID(const std::vector<T>& vec, int32_t id)
{
	for (const auto& data : vec)
		if (data.id == id)
			return &data;
	return nullptr;
}

static int32_t GetConfigSetID()
{
	int32_t set = *reinterpret_cast<int32_t*>(game::GetSaveData() + 0x169410);
	return set < 3 ? -(set + 1) : game::GetGlobalPVInfo()->pv_id;
}

namespace customize_sel
{
	struct SoundConfig
	{
		const char* name;
		int8_t* selected_id;
		const std::vector<SoundInfo>* sounds;
		int32_t base_type; // 0 - None, 1 - Button, 2 - Star
		bool has_song_default;
		bool play_preview_on_chosen;
	};

	struct SetOption
	{
		int32_t id;
		std::string name;
	};

	struct CSStateConfigNC
	{
		bool window_open = false;
		int32_t selected_set = -1;
		std::vector<SetOption> sets;
		std::vector<SoundConfig> sound_nodes;
		std::vector<std::string> tech_zone_styles;
		bool assets_loaded = false;
		bool assets_loading = false;
	} static cs_state;

	static void RepopulateSetNodes()
	{
		cs_state.sets.clear();
		cs_state.sets.push_back({ -1, "Shared Set A" });
		cs_state.sets.push_back({ -2, "Shared Set B" });
		cs_state.sets.push_back({ -3, "Shared Set C" });
		cs_state.sets.push_back({ game::GetGlobalPVInfo()->pv_id, "Song Specific" });
	}

	static void RepopulateSoundNodes()
	{
		auto addSoundNode = [&](const char* name, int8_t* selected_id, const std::vector<SoundInfo>* sounds, int32_t base, bool has_song_default = false, bool preview = true)
		{
			SoundConfig& node = cs_state.sound_nodes.emplace_back();
			node.name = name;
			node.selected_id = selected_id;
			node.sounds = sounds;
			node.base_type = base;
			node.has_song_default = has_song_default;
			node.play_preview_on_chosen = preview;
		};

		ConfigSet* set = nc::FindConfigSet(cs_state.selected_set, true);

		cs_state.sound_nodes.clear();
		addSoundNode("W Button", &set->button_w_se_id, sound_db::GetButtonWSoundDB(), 1);
		addSoundNode("Sustain", &set->button_l_se_id, sound_db::GetButtonLongOnSoundDB(), 1, false, false);
		addSoundNode("Star", &set->star_se_id, sound_db::GetStarSoundDB(), 0);
		addSoundNode("W Star", &set->star_w_se_id, sound_db::GetStarWSoundDB(), 2);
	}

	static void OpenWindow()
	{
		cs_state.window_open = true;
		cs_state.selected_set = GetConfigSetID();
		RepopulateSetNodes();
		RepopulateSoundNodes();

		/*
		cs_state.tech_zone_styles.clear();
		cs_state.tech_zone_styles.push_back("F");
		cs_state.tech_zone_styles.push_back("F 2nd");
		cs_state.tech_zone_styles.push_back("X");
		*/

		// TODO: Block inputs
		// TODO: Set screen fade (?)
	}

	static void CloseWindow()
	{
		cs_state.window_open = false;
		cs_state.selected_set = -1;
		cs_state.sets.clear();
		cs_state.sound_nodes.clear();
		// TODO: Unblock inputs
		// TODO: Reset screen fade
	}

	static std::string GetSameAsSoundName(const SoundConfig& config)
	{
		if (config.base_type == 1)
			return "Same as Button";
		else if (config.base_type == 2)
			return "Same as Star";
		return "(NULL)";
	}

	static std::string GetSoundNodePreviewText(const SoundConfig& config)
	{
		switch (*config.selected_id)
		{
		case -2:
			return "Songs Default";
		case -1:
			return GetSameAsSoundName(config);
		default:
			if (const auto* snd = FindWithID(*config.sounds, *config.selected_id); snd != nullptr)
				return snd->name.c_str();
			break;
		}

		return "(NULL)";
	}

	static void PlaySoundPreview(const SoundInfo& snd)
	{
		std::string preview_se = snd.se_name;
		if (!snd.se_preview_name.empty())
			preview_se = snd.se_preview_name;

		if (!preview_se.empty())
			sound::PlaySoundEffect(3, preview_se.c_str(), 1.0f);
	}

	static void ShowSoundNode(SoundConfig& config)
	{
		ImGui::PushID(config.name);

		// TODO: Add a better way to align elements;
		//       This won't work if we change the font to a non-fixed size one in the future
		ImGui::Text("%-12s", config.name);
		ImGui::SameLine();

		std::string preview_name = GetSoundNodePreviewText(config);
		if (ImGui::BeginCombo("##sound-set", preview_name.c_str()))
		{
			if (config.has_song_default)
			{
				if (ImGui::Selectable("Song Defaults", *config.selected_id == -2))
					*config.selected_id = -2;
			}

			if (config.base_type != 0)
			{
				if (ImGui::Selectable(GetSameAsSoundName(config).c_str(), *config.selected_id == -1))
					*config.selected_id = -1;
			}

			for (const auto& sound : *config.sounds)
			{
				if (ImGui::Selectable(sound.name.c_str(), *config.selected_id == sound.id, ImGuiSelectableFlags_AllowItemOverlap))
				{
					*config.selected_id = sound.id;
					if (config.play_preview_on_chosen)
						PlaySoundPreview(sound);
				}

				ImGui::PushID(sound.name.c_str());

				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 30.0f);
				if (ImGui::SmallButton(u8"")) // NOTE: UTF-8 "Play" symbol
					PlaySoundPreview(sound);

				ImGui::PopID();
			}

			ImGui::EndCombo();
		}

		ImGui::PopID();
	}

	void ShowWindow()
	{
		if (!cs_state.window_open)
			return;

		bool open = true;
		if (ImGui::Begin("NewClassics", &open, ImGuiWindowFlags_NoCollapse))
		{
			// NOTE: Show config set combo box
			if (ImGui::BeginCombo("##set-combo", FindWithID(cs_state.sets, cs_state.selected_set)->name.c_str()))
			{
				for (auto& opt : cs_state.sets)
				{
					if (ImGui::Selectable(opt.name.c_str(), opt.id == cs_state.selected_set))
					{
						cs_state.selected_set = opt.id;
						RepopulateSoundNodes();
					}
				}

				ImGui::EndCombo();
			}

			// NOTE: Show tabs
			if (ImGui::BeginTabBar("##tabs"))
			{
				if (ImGui::BeginTabItem("Sounds"))
				{
					for (auto& node : cs_state.sound_nodes)
						ShowSoundNode(node);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Other"))
				{
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::End();
		}

		if (!open)
			CloseWindow();
	}
}

HOOK(bool, __fastcall, CustomizeSelTaskInit, 0x140687D10, uint64_t a1)
{
	customize_sel::cs_state.assets_loaded = false;
	customize_sel::cs_state.assets_loading = false;

	sound::RequestFarcLoad("rom/sound/se_nc.farc");
	sound::RequestFarcLoad("rom/sound/se_nc_option.farc");
	customize_sel::cs_state.assets_loading = true;

	return originalCustomizeSelTaskInit(a1);
}

HOOK(bool, __fastcall, CustomizeSelTaskCtrl, 0x140687D70, uint64_t a1)
{
	if (customize_sel::cs_state.assets_loading)
	{
		customize_sel::cs_state.assets_loaded = !sound::IsFarcLoading("rom/sound/se_nc.farc") && !sound::IsFarcLoading("rom/sound/se_nc_option.farc");
		if (customize_sel::cs_state.assets_loaded)
			customize_sel::cs_state.assets_loading = false;
	}

	return originalCustomizeSelTaskCtrl(a1);
}

HOOK(bool, __fastcall, CustomizeSelTaskDest, 0x140687D80, uint64_t a1)
{
	sound::UnloadFarc("rom/sound/se_nc.farc");
	sound::UnloadFarc("rom/sound/se_nc_option.farc");
	return originalCustomizeSelTaskDest(a1);
}

HOOK(void, __fastcall, CSTopMenuMainCtrl, 0x14069B610, uint64_t a1)
{
	diva::InputState* is = diva::GetInputState(0);
	if (is->IsButtonTapped(92) || (is->GetDevice() != InputDevice_Keyboard && is->IsButtonTapped(13)))
		customize_sel::OpenWindow();

	originalCSTopMenuMainCtrl(a1);
}

void InstallCustomizeSelHooks()
{
	INSTALL_HOOK(CustomizeSelTaskInit);
	INSTALL_HOOK(CustomizeSelTaskCtrl);
	INSTALL_HOOK(CustomizeSelTaskDest);
	INSTALL_HOOK(CSTopMenuMainCtrl);
}