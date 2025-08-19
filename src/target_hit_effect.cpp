#include <target_hit_effect.h>

namespace hiteff {
	uint32_t cur_hit_eff_aetset_id = 0xFFFFFFFF;
	uint32_t cur_hit_eff_scene_id = 0xFFFFFFFF;
	uint32_t cur_hit_eff_sprset_id = 0xFFFFFFFF;
	//AetComposition cur_hit_eff_compo;
	std::map<int32_t, std::string> fail_target_effect_map;
	std::map<int32_t, std::string> success_target_effect_map;
}