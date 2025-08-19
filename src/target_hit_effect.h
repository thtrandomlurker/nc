#include <stdint.h>
#include "diva.h"
#include <vector>

namespace hiteff {
	extern uint32_t cur_hit_eff_aetset_id;
	extern uint32_t cur_hit_eff_scene_id;
	extern uint32_t cur_hit_eff_sprset_id;
	//extern AetComposition cur_hit_eff_compo;
	extern std::map<int32_t, std::string> fail_target_effect_map;
	extern std::map<int32_t, std::string> success_target_effect_map;
}