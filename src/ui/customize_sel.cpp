#include <stdint.h>
#include <vector>
#include <diva.h>
#include <hooks.h>
#include <nc_log.h>
#include <sound_db.h>
#include <save_data.h>
#include <helpers.h>
#include <input.h>
#include <util.h>
#include "common.h"
#include "customize_sel.h"

struct CSStateConfigNC
{
	bool window_open = false;
	bool assets_loaded = false;
	int32_t aet_scene_id = 14010150;
} static cs_state;

struct SoundOptionInfo
{
	int32_t id = 0;
	std::string preview_name;
};

struct SelectorExtraData
{
	int32_t same_index = -1;
	int32_t song_default_index = -1;
	int32_t base_index = 0;
	std::vector<SoundOptionInfo> sounds;
};

static void PlayPreviewSoundEffect(HorizontalSelector* sel_base, int32_t index, const void* extra)
{
	HorizontalSelectorMulti* sel = dynamic_cast<HorizontalSelectorMulti*>(sel_base);
	const auto* ex_data = reinterpret_cast<const SelectorExtraData*>(extra);

	sound::PlaySoundEffect(3, ex_data->sounds[sel->selected_index].preview_name.c_str(), 1.0f);
}

static void StoreSoundEffectConfig(int32_t index, const SelectorExtraData& ex_data, int8_t* output)
{
	int32_t id = ex_data.sounds[index].id;
	if (index == ex_data.song_default_index)
		id = -2;
	else if (index == ex_data.same_index)
		id = -1;

	*output = id;
}

class NCConfigWindow : public AetControl
{
protected:
	bool finishing = false;
	bool exit = false;
	int32_t prev_selected_tab = 0;
	int32_t selected_tab = 0;
	int32_t selected_option = 0;
	AetElement sub_menu_base;
	std::vector<std::unique_ptr<HorizontalSelector>> selectors;
	std::vector<SelectorExtraData> user_data;
	float win_opacity = 1.0f;
	ConfigSet* config_set;

	static constexpr int32_t WindowPrio = 18;
	static constexpr int32_t MaxTabCount = 2;
	static constexpr uint32_t NumberSprites[3] = { 880817216, 1732835926, 3315147794 };
	static constexpr uint32_t TabInfoSprites[MaxTabCount] = { 2099321196, 2806350346 };

public:
	NCConfigWindow()
	{
		AllowInputsWhenBlocked(true);
		config_set = nc::GetConfigSet();

		SetScene(cs_state.aet_scene_id);
		SetLayer("cmn_win_nc_options_g_inout", 0x10000, WindowPrio, 14, AetAction_InLoop);
		ChangeTab(0);
	}

	bool ShouldExit() const { return exit; }

	void Ctrl() override
	{
		AetControl::Ctrl();

		if (finishing && Ended())
		{
			exit = true;
			return;
		}

		if (auto layout = GetLayout("nswgam_cmn_win_base.pic"); layout.has_value())
			win_opacity = layout.value().opacity;

		for (auto& selector : selectors)
		{
			selector->SetOpacity(win_opacity);
			selector->text_opacity = win_opacity;
			selector->Ctrl();
		}
	}

	void Disp() override
	{
		for (auto& selector : selectors)
			selector->Disp();

		DrawSpriteAt("p_nc_page_num_10_c", NumberSprites[selected_tab + 1]);
		DrawSpriteAt("p_nc_page_num_01_c", NumberSprites[MaxTabCount]);
		DrawSpriteAt("p_nc_img_02_c", TabInfoSprites[prev_selected_tab]);
		DrawSpriteAt("p_nc_img_01_c", TabInfoSprites[selected_tab]);
	}

	void OnActionPressed(int32_t action) override
	{
		if (finishing)
			return;

		switch (action)
		{
		case KeyAction_MoveUp:
			SetSelectorIndex(-1, true);
			sound::PlaySelectSE();
			break;
		case KeyAction_MoveDown:
			SetSelectorIndex(1, true);
			sound::PlaySelectSE();
			break;
		case KeyAction_SwapLeft:
			ChangeTab(-1);
			break;
		case KeyAction_SwapRight:
			ChangeTab(1);
			break;
		case KeyAction_Cancel:
			SetLayer("cmn_win_nc_options_g_inout", 0x10000, WindowPrio, 14, AetAction_OutOnce);
			finishing = true;
			break;
		}
	}

	HorizontalSelectorMulti* CreateMultiOptionElement(int32_t id, int32_t loc_id, std::function<void(int32_t, const std::string&)> func = nullptr)
	{
		diva::vec3 pos = { 0.0f, 0.0f, 0.0f };

		if (auto layout = sub_menu_base.GetLayout(util::Format("p_nc_submenu_%02d_c", loc_id)); layout.has_value())
			pos = layout.value().position;

		auto opt = std::make_unique<HorizontalSelectorMulti>(
			cs_state.aet_scene_id,
			util::Format("option_submenu_nc_%02d__f", id),
			WindowPrio,
			14
		);

		opt->AllowInputsWhenBlocked(true);
		opt->SetPosition(pos);

		if (func)
			opt->SetOnChangeNotifier(func);

		selectors.push_back(std::move(opt));
		return dynamic_cast<HorizontalSelectorMulti*>(selectors.back().get());
	}

	void ChangeTab(int32_t dir)
	{
		prev_selected_tab = selected_tab;
		selected_tab += dir;
		if (selected_tab < 0)
			selected_tab = MaxTabCount - 1;
		else if (selected_tab >= MaxTabCount)
			selected_tab = 0;

		sub_menu_base.SetScene(cs_state.aet_scene_id);
		sub_menu_base.SetLayer(util::Format("submenu_nc_anm_%02d", selected_tab + 1), 0x10000, 15, 14, "", "", nullptr);

		auto putSoundEffectList = [&](int32_t id, int32_t loc_id, const std::vector<SoundInfo>& sounds, int32_t same_id, int8_t* selected_id)
		{
			auto& ex_data = user_data.emplace_back();
			size_t index = selectors.size();

			auto* opt = CreateMultiOptionElement(
				id,
				loc_id,
				[this, index, selected_id](int32_t i, const std::string&) { StoreSoundEffectConfig(i, user_data[index], selected_id); }
			);

			if (same_id > 0)
			{
				if (*selected_id == -1)
					opt->selected_index = static_cast<int32_t>(opt->values.size());

				ex_data.same_index = static_cast<int32_t>(opt->values.size());
				ex_data.sounds.emplace_back().id = -1;
				ex_data.base_index++;

				opt->values.emplace_back(loc::GetString(6250 + same_id));
			}

			for (const auto& snd : sounds)
			{
				if (snd.id == *selected_id)
					opt->selected_index = static_cast<int32_t>(opt->values.size());

				auto& info = ex_data.sounds.emplace_back();
				info.id = snd.id;
				info.preview_name = !snd.se_preview_name.empty() ? snd.se_preview_name : snd.se_name;
				
				opt->values.push_back(snd.name);
			}

			opt->SetExtraData(&user_data[index]);
			opt->SetPreviewNotifier(PlayPreviewSoundEffect);
		};

		selectors.clear();
		user_data.clear();
		user_data.reserve(20);
		if (selected_tab == 0)
		{
			// TODO: Change the sound effect names (maybe?) and the "Same as" text to STR ARRAY
			//       string to allow localization.
			putSoundEffectList(1, 1, *sound_db::GetButtonLongOnSoundDB(), 1, &config_set->button_l_se_id);
			putSoundEffectList(2, 2, *sound_db::GetButtonWSoundDB(),      1, &config_set->button_w_se_id);
			putSoundEffectList(3, 3, *sound_db::GetStarSoundDB(),         0, &config_set->star_se_id);
			putSoundEffectList(4, 4, *sound_db::GetButtonLongOnSoundDB(), 2, &config_set->link_se_id); 
			putSoundEffectList(5, 5, *sound_db::GetStarWSoundDB(),        2, &config_set->star_w_se_id);
		}

		SetSelectorIndex(0);
	}

	void SetSelectorIndex(int32_t index, bool relative = false)
	{
		selected_option = relative ? selected_option + index : index;
		if (selected_option < 0)
			selected_option = selectors.size() - 1;
		else if (selected_option >= selectors.size())
			selected_option = 0;

		for (size_t i = 0; i < selectors.size(); i++)
			selectors[i]->SetFocus(i == selected_option);
	}
};

namespace customize_sel
{
	std::unique_ptr<NCConfigWindow> window;

	static bool CtrlWindow()
	{
		if (!cs_state.window_open)
		{
			diva::InputState* is = diva::GetInputState(0);
			if (is->IsButtonTapped(92) || is->IsButtonTapped(13))
			{
				window = std::make_unique<NCConfigWindow>();
				cs_state.window_open = true;
				nc::BlockInputs();
			}
		}

		if (cs_state.window_open)
		{
			window->Ctrl();
			if (window->ShouldExit())
			{
				window.reset();
				cs_state.window_open = false;
				nc::UnblockInputs();
			}
		}

		return cs_state.window_open;
	}
}

HOOK(bool, __fastcall, CustomizeSelTaskInit, 0x140687D10, uint64_t a1)
{
	sound::RequestFarcLoad("rom/sound/se_nc.farc");
	sound::RequestFarcLoad("rom/sound/se_nc_option.farc");

	prj::string out;
	prj::string_view out2;
	aet::LoadAetSet(14010060, &out);
	spr::LoadSprSet(14020060, &out2);

	cs_state.assets_loaded = false;
	return originalCustomizeSelTaskInit(a1);
}

HOOK(bool, __fastcall, CustomizeSelTaskCtrl, 0x140687D70, uint64_t a1)
{
	if (!cs_state.assets_loaded)
	{
		cs_state.assets_loaded = !sound::IsFarcLoading("rom/sound/se_nc.farc")
			&& !sound::IsFarcLoading("rom/sound/se_nc_option.farc")
			&& !aet::CheckAetSetLoading(14010060)
			&& !spr::CheckSprSetLoading(14020060);
	}

	return originalCustomizeSelTaskCtrl(a1);
}

HOOK(bool, __fastcall, CustomizeSelTaskDest, 0x140687D80, uint64_t a1)
{
	customize_sel::window.reset();
	sound::UnloadFarc("rom/sound/se_nc.farc");
	sound::UnloadFarc("rom/sound/se_nc_option.farc");
	aet::UnloadAetSet(14010060);
	spr::UnloadSprSet(14020060);
	cs_state.assets_loaded = false;
	return originalCustomizeSelTaskDest(a1);
}

HOOK(void, __fastcall, CustomizeSelTaskDisp, 0x140687DE0, uint64_t a1)
{
	originalCustomizeSelTaskDisp(a1);
	if (customize_sel::window)
		customize_sel::window->Disp();
}

HOOK(void, __fastcall, CSTopMenuMainCtrl, 0x14069B610, uint64_t a1)
{
	customize_sel::CtrlWindow();
	originalCSTopMenuMainCtrl(a1);
}

void InstallCustomizeSelHooks()
{
	INSTALL_HOOK(CustomizeSelTaskInit);
	INSTALL_HOOK(CustomizeSelTaskCtrl);
	INSTALL_HOOK(CustomizeSelTaskDest);
	INSTALL_HOOK(CustomizeSelTaskDisp);
	INSTALL_HOOK(CSTopMenuMainCtrl);
}