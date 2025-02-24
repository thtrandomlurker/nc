#pragma once

#include <stdint.h>
#include "diva.h"

// NOTE: Some information were taken from ReDIVA. Thank you, Koren!
//		 https://github.com/korenkonder/ReDIVA/blob/master/src/ReDIVA/pv_game/pv_game_pv_data.hpp
//
struct PvDscTarget
{
	int32_t type;
	diva::vec2 target_pos;
	diva::vec2 start_pos;
	float amplitude;
	int32_t frequency;
	bool slide_chain_start;
	bool slide_chain_end;
	bool slide_chain_left;
	bool slide_chain_right;
	bool slide_chain;
};

struct PvDscTargetGroup
{
	int32_t target_count;
	PvDscTarget targets[4];
	int32_t field_94;
	int64_t spawn_time;
	int64_t hit_time;
	bool slide_chain;
};

struct PVGamePvData
{
	uint8_t gap0[4];
	int32_t dsc_state;
	uint8_t byte8[4];
	int32_t script_buffer[45000];
	int32_t script_pos;
	int64_t qword2BF30;
	int64_t qword2BF38;
	int64_t cur_time;
	float cur_time_float;
	float prev_time_float;
	uint8_t gap2BF50[32];
	int int2BF70;
	uint8_t gap2BF74[36];
	float float2BF98;
	float float2BF9C;
	int32_t dword2BFA0;
	int32_t dword2BFA4;
	uint64_t script_time;
	uint8_t gap2BFA8[16];
	uint8_t byte2BFC0;
	uint8_t byte2BFC1;
	__declspec(align(8)) uint8_t byte2BFC8;
	int32_t dword2BFCC;
	int32_t dword2BFD0;
	uint8_t byte2BFD4;
	__declspec(align(2)) uint8_t byte2BFD6;
	uint8_t gap2BFD7[961];
	uint8_t byte2C398;
	int32_t dword2C39C;
	uint8_t gap2C3A0[8];
	uint8_t byte2C3A8;
	int32_t dword2C3AC;
	uint8_t gap2C3B0[272];
	uint8_t byte2C4C0;
	int32_t dword2C4C4;
	float float2C4C8;
	float float2C4CC;
	float float2C4D0;
	float float2C4D4;
	float float2C4D8;
	float float2C4DC;
	uint8_t gap2C4E0;
	unsigned __int8 unsigned___int82C4E1;
	int32_t targets_remaining;
	prj::vector<PvDscTargetGroup> targets;
	size_t target_index;
	float float2C508;
	char char2C50C;
	uint8_t gap2C50D[63];
	float float2C54C;
	uint8_t gap2C550[4];
	unsigned int unsigned_int2C554;
	uint8_t gap2C558[712];
	uint8_t byte2C820;
	int32_t dword2C824;
	int32_t dword2C828;
	float float2C82C;
};

struct PvGameTarget
{
	PvGameTarget* prev;
	PvGameTarget* next;
	int32_t dword10;
	int32_t target_type;
	float flying_time_remaining;
	uint8_t gap1C[4];
	float flying_time;
	float amplitude;
	float freq;
	float cur_freq;
	uint8_t gap30[4];
	int32_t dword34;
	uint8_t gap38[4];
	float dword3C;
	uint8_t gap40;
	bool slide_chain_start;
	bool slide_chain_end;
	bool slide_left;
	bool slide_right;
	bool slide_chain;
	uint8_t gap46[2];
	int64_t hit_time;
	uint8_t gap50[32];
	int32_t target_aet;
	int32_t button_aet;
	int32_t dword78;
	int32_t target_eff_aet;
	diva::vec2 target_pos;
	diva::vec2 button_pos;
	diva::vec2 delta_pos;
	diva::vec2 delta_pos_sq;
	diva::vec2 vecA0;
	SpriteVertex kiseki[40];
	bool note_active;
	uint8_t gap469[3];
	float out_start_time;
	uint8_t gap470[4];
	int32_t hit_state;
	int32_t multi_count;
	int32_t dword47C;
	float float480;
	uint8_t gap484[12];
	uint8_t byte490;
	bool b491;
	bool b492;
	bool b493;
	uint8_t gap494[12];
	float scaling_end_time;
	uint8_t gap4A4[4];
	int32_t sprite_index;
	uint8_t gap4AC[4];
	int64_t appear_time;
	int16_t word4B8;
	uint8_t gap4BA[2];
	float kiseki_width;
	int32_t target_index;
	int32_t dword4C4;
};

struct PVGameArcade
{
	bool play;
	uint8_t gap1[7];
	void* ptr08;
	uint8_t gap[16];
	PvGameTarget* target;
	PvGameTarget target_buffer[64];
	uint8_t gap13228[64];
	float target_time;
	float cool_early_window;
	float cool_late_window;
	float fine_early_window;
	float fine_late_window;
	float safe_early_window;
	float safe_late_window;
	float sad_early_window;
	float sad_late_window;
	uint8_t gap1328C[4];
	float current_time;
	int32_t int13294;
	int32_t hit_effects[64];
	int32_t hit_effect_index;
	uint8_t gap1339C[96];
	int32_t dword133FC;
	int32_t dword13400[3];
	int32_t slide_hit_effects[64];
	int32_t slide_hit_effect_index;
	uint8_t gap13510[8];
	float fl13518;
	float fl1351C;
	float fl13520;
	float fl13524;
	float fl13528;
	float fl1352C;
	float fl13530;
	uint8_t gap13534[8];
	bool bool1353C[4];
	uint8_t gap13540[14];
	int8_t char1354E[4];
	float fl13554[2];
};

struct PVGameData
{
	bool loaded;
	bool paused;
	bool byte2;
	bool byte3;
	uint8_t gap4[196];
	PVGamePvData pv_data;
	uint8_t gap2C8F8[1528];
	uint8_t byte2CEF0;
	uint8_t gap2CEF1[39];
	const char char2CF18;
	uint8_t gap2CF19[23];
	int64_t qword2CF30;
	const char char2CF38;
	uint8_t gap2CF39[23];
	int64_t qword2CF50;
	const char char2CF58;
	uint8_t gap2CF59[23];
	int64_t qword2CF70;
	const char char2CF78;
	uint8_t gap2CF79[23];
	int64_t qword2CF90;
	const char char2CF98;
	uint8_t gap2CF99[23];
	int64_t qword2CFB0;
	const char char2CFB8;
	uint8_t gap2CFB9[23];
	int64_t qword2CFD0;
	const char char2CFD8;
	uint8_t gap2CFD9[23];
	int64_t qword2CFF0;
	const char char2CFF8;
	uint8_t gap2CFF9[23];
	int64_t qword2D010;
	uint8_t gap2D018[357];
	uint8_t byte2D17D;
	uint8_t gap2D17E[182];
	int life;
	int32_t dword2D238;
	uint8_t gap2D23C[4];
	uint8_t byte2D240;
	uint8_t gap2D241[7];
	unsigned int unsigned_int2D248;
	int32_t dword2D24C;
	unsigned int unsigned_int2D250;
	int32_t dword2D254;
	uint8_t gap2D258[64];
	float float2D298;
	uint8_t gap2D29C[12];
	int int2D2A8;
	uint8_t gap2D2AC;
	uint8_t byte2D2AD;
	float float2D2B0;
	int16_t word2D2B4;
	uint8_t gap2D2B6[10];
	int32_t dword2D2C0;
	uint8_t gap2D2C4[8];
	int32_t reference_score;
	int32_t reference_score_with_life;
	int32_t dword2D2D4;
	prj::vector<int32_t> target_reference_scores;
	uint8_t gap2D2F0[8];
	int64_t qword2D2F8;
	uint8_t gap2D300[29];
	uint8_t byte2D31D;
	uint8_t gap2D31E;
	uint8_t byte2D31F;
	int64_t challenge_time_start;
	int64_t challenge_time_end;
	int64_t pv_end_time;
	float pv_end_time_sec;
	float float2D33C;
	uint8_t gap2D340[4];
	float float2D344;
	int64_t qword2D348;
	float float2D350;
	float float2D354;
	int32_t dword2D358;
	int32_t dword2D35C;
	uint8_t gap2D360[16];
	int32_t dword2D370;
	uint8_t byte2D374;
	uint8_t byte2D375;
	uint8_t gap2D376[30];
	int32_t current_frame;
	int32_t dword2D398;
	int32_t dword2D39C;
	int32_t dword2D3A0;
	uint8_t gap2D3A4[5];
	uint8_t byte2D3A9;
	uint8_t gap2D3AA;
	uint8_t byte2D3AB;
	uint8_t gap2D3AC[2];
	uint8_t byte2D3AE;
	bool is_success_branch;
	uint8_t gap2D3B0[16];
	int32_t dword2D3C0;
	uint8_t gap2D3C4[16];
	uint8_t byte2D3D4;
	uint8_t gap2D3D5[19];
	int32_t dword2D3E8;
	uint8_t gap2D3EC[2020];
	int64_t qword2DBD0;
	int64_t qword2DBD8;
	float float2DBE0;
	int32_t dword2DBE4;
	uint8_t byte2DBE8;
	uint8_t byte2DBE9;
	uint8_t byte2DBEA;
	float float2DBEC;
	uint8_t byte2DBF0;
	uint8_t gap2DBF1[11];
	float float2DBFC;
	uint8_t gap2DC00[684];
	int64_t int642DEAC;
};

struct SoundEffect
{
	prj::string button;
	prj::string slide;
	prj::string slidechain;
	prj::string slide_button;
	prj::string slide_ok;
	prj::string slide_ng;
	prj::string chime;
};

inline FUNCTION_PTR(int32_t, __fastcall, FindNextCommand, 0x140257D50, PVGamePvData* pv_data, int32_t op, int32_t* time, int32_t* branch, int32_t head);
inline FUNCTION_PTR(PVGameData*, __fastcall, GetPVGameData, 0x140266720);
inline FUNCTION_PTR(bool, __fastcall, IsInSongResults, 0x1401E8090);

inline bool ShouldUpdateTargets()
{
	PVGameData* pv_game = GetPVGameData();
	if (pv_game != nullptr)
		return !IsInSongResults() && !pv_game->paused && !pv_game->byte2;

	return false;
}
