#pragma once

#include <stdint.h>
#include <string.h>

// NOTE: Shared sets or song-specific configuration
struct ConfigSet
{
	// + ----------------------------------- +
	// |    -2 = Song Default                |
	// |    -1 = Same as <Button / Star>     |
	// |    >0 = Sound DB ID                 |
	// + ----------------------------------- +
	int8_t button_w_se_id  = -1;
	int8_t button_l_se_id  = -1;
	int8_t star_se_id      = -2;
	int8_t star_w_se_id    = -1;
	int8_t star_l_se_id    = -1;
	int8_t link_se_id      = -1;
	int8_t rush_se_id      = -1; // NOTE: No real reason to make this modifiable, I think
	int8_t tech_zone_style =  1;
	// NOTE: Negative IDs are shared sets (-1 = Set A; -2 = Set B; -3 = Set C)
	//       Positive IDs are treated as PV IDs.
	//       This is only relevant when writing and parsing the file.
	int32_t id = 0;
	uint8_t reserved[52];

	ConfigSet() { memset(reserved, 0, sizeof(reserved)); }
};

struct SharedData
{
	int32_t pv_sel_selected_style;
	uint8_t reserved[252];

	SharedData()
	{
		pv_sel_selected_style = 0;
		memset(reserved, 0, sizeof(reserved));
	}
};

static_assert(sizeof(ConfigSet) == 64, "ConfigSet struct size mismatch.");
static_assert(sizeof(SharedData) == 256, "SharedData struct size mismatch.");

namespace nc
{
	ConfigSet* FindConfigSet(int32_t id, bool create_if_missing = false);
	void CreateDefaultSaveData();
	SharedData& GetSharedData();

	int32_t GetConfigSetID();
	ConfigSet& GetConfigSet();

	void InstallSaveDataHooks();
}