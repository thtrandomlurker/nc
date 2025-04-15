#pragma once

#include <string>
#include <memory>
#include <nc_state.h>
#include <diva.h>

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
	};

	class GSWindow
	{
	private:
		static constexpr size_t MaxOptionCount = 4;

		int32_t option_count;
		uint8_t options[MaxOptionCount];
		AetHandle base_win;
		AetHandle base_txt;
		AetHandle style_txt_buffer[2];
		int32_t style_txt_index;
		int32_t selected_index;
		int32_t previous_option;
		bool hidden;
		bool dirty;
		bool aet_dirty;

		std::string GetModePrefix();
		std::string GetLanguageSuffix();
		std::string GetBaseLayerName();
		std::string GetBaseTextLayerName();
		std::string GetStyleTextLayerName(int32_t style);

	public:
		bool is_sort_mode;

		GSWindow()
		{
			base_win = 0;
			base_txt = 0;
			style_txt_buffer[0] = 0;
			style_txt_buffer[1] = 0;
			is_sort_mode = false;
			hidden = false;
			dirty = false;
			aet_dirty = false;
			Reset();
		}

		~GSWindow()
		{
			Reset();
		}

		void Reset();
		void SetOptions(const uint8_t* opts, int32_t count, bool clean = false);
		void ForceSetOption(int32_t opt);
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

		inline void MakeAetDirty() { aet_dirty = true; }
	};

	inline std::unique_ptr<GSWindow> gs_win;

	void RequestAssetsLoad();
	bool CheckAssetsLoaded();
	void UnloadAssets();

	inline int32_t GetSelectedStyleOrDefault()
	{
		if (gs_win)
		{
			if (int32_t style = gs_win->GetSelectedStyle(); style != GameStyle_Max)
				return style;
		}

		return GameStyle_Arcade;
	}

	std::vector<uint8_t> FindAvailableStyles(const PvData* const* pvs, size_t count, int32_t difficulty, int32_t edition);
	inline std::vector<uint8_t> FindAvailableStyles(const prj::vector<PvData*>& pvs, int32_t difficulty, int32_t edition)
	{
		return FindAvailableStyles(pvs.data(), pvs.size(), difficulty, edition);
	}

	bool IsSongAvailableInCurrentStyle(const PvData* pv_data, int32_t difficulty, int32_t edition);
}

// NOTE: Must be called in PostInit to properly figure out if it's MM or FT UI
void InstallPvSelHooks();