#pragma once

#include <array>
#include <string>
#include <map>
#include <memory>
#include <nc_state.h>
#include <diva.h>
#include "common.h"

namespace pvsel
{
	// NOTE: Common structs used in both PVSelectorSwitch and PVsel (PS4);
	//       You can noticed that I haven't thought of good names for them yet.
	struct PvData2
	{
		int32_t* pv_db_entry;
		uint8_t gap08[120];
		int32_t name_sort_index;
	};

	struct PvData
	{
		PvData2* data2;
		uint8_t data[0x40];
	};

	struct SelPvList
	{
		uint8_t gap00[0xF0];
		prj::vector<PvData*>* pv_data;
		uint8_t gapF8[0x28];

		inline size_t GetSongCount() const
		{
			if (pv_data)
				return pv_data->size();
			return 0;
		}
	};

	class GSWindow
	{
	private:
		static constexpr size_t MaxOptionCount = 4;

		int32_t option_count;
		uint8_t options[MaxOptionCount];
		AetElement base_win;
		AetElement base_txt;
		AetElement prev_style_txt;
		AetElement cur_style_txt;
		int32_t selected_index;
		int32_t previous_option;
		int32_t preferred_style;
		bool hidden;
		bool dirty;

		std::string GetModePrefix();
		std::string GetLanguageSuffix();
		std::string GetBaseLayerName();
		std::string GetBaseTextLayerName();
		std::string GetStyleTextLayerName(int32_t style);

	public:
		GSWindow()
		{
			preferred_style = GameStyle_Max;
			previous_option = GameStyle_Max;
			selected_index = 0;
			option_count = 0;
			memset(options, GameStyle_Max, MaxOptionCount);
			hidden = false;
			dirty = false;
		}

		bool SetAvailableOptions(std::array<int32_t, GameStyle_Max> song_counts);
		void UpdateAet();
		void SetVisible(bool visible);
		bool Ctrl();
		void Disp() const;

		inline bool IsToggleable() const { return option_count > 1; }
		inline int32_t GetSelectedStyle() const
		{
			if (selected_index >= 0 && selected_index < MaxOptionCount)
				return options[selected_index];
			return GameStyle_Max;
		}

		inline void SetPreferredStyle(int32_t style) { preferred_style = style; }
		inline int32_t GetPreferredStyle() const { return preferred_style; }
	};

	inline std::unique_ptr<GSWindow> gs_win;

	void RequestAssetsLoad();
	bool CheckAssetsLoaded();
	void UnloadAssets();

	int32_t GetSelectedStyleOrDefault();
	int32_t GetPreferredStyleOrDefault();
	bool CheckSongHasStyleAvailable(int32_t pv, int32_t difficulty, int32_t edition, int32_t style);

	template <typename T>
	std::array<int32_t, GameStyle_Max> GetSongCountPerStyle(T* sel)
	{
		std::array<int32_t, GameStyle_Max> song_counts = { };
		song_counts.fill(0);

		if (!sel->sel_pv_list.pv_data)
			return song_counts;

		for (const pvsel::PvData* pv : *sel->sel_pv_list.pv_data)
		{
			if (!pv->data2)
				continue;

			for (int32_t style = 0; style < GameStyle_Max; style++)
			{
				if (pvsel::CheckSongHasStyleAvailable(*pv->data2->pv_db_entry, sel->difficulty, sel->edition, style))
					song_counts[style]++;
			}
		}

		return song_counts;
	}

	template <typename T>
	int32_t GetSelectedPVIndex(T* sel)
	{
		if (sel->sel_pv_list.pv_data)
		{
			int32_t index = 0;
			for (const auto& song : *sel->sel_pv_list.pv_data)
			{
				if (song->data2)
					if (*song->data2->pv_db_entry == sel->pv_id)
						return index;
				index++;
			}
		}

		return 0;
	}

	template <typename T, typename F>
	void CalculateAllSongCount(T* sel, F pertains)
	{
		int32_t* count = &sel->song_counts[sel->all_sort_index];
		*count = 0;

		for (const auto& pv : sel->sorted_pv_lists[2 * 4 * sel->sort_mode + 2 * sel->difficulty + sel->edition])
		{
			if (!pv.data2)
				continue;

			if (pertains(sel, &pv, sel->all_sort_index, sel->all_sort_index))
			{
				if (pvsel::CheckSongHasStyleAvailable(*pv.data2->pv_db_entry, sel->difficulty, sel->edition, pvsel::GetSelectedStyleOrDefault()))
					++(*count);
			}
		}
	}

	prj::vector<pvsel::PvData*> SortWithStyle(const SelPvList& list, int32_t difficulty, int32_t edition, int32_t style);
}

// NOTE: Must be called in PostInit to properly figure out if it's MM or FT UI
void InstallPvSelHooks();