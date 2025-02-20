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

	/*
	_BYTE gap13228[64];
	float target_flying_time;
	_BYTE gap1326C[44];
	int32_t hit_effects[64];
	int32_t hit_effect_index;
	_BYTE gap1339C[96];
	int32_t dword133FC;
	int32_t dword13400[3];
	int32_t slide_hit_effects[64];
	int32_t slide_hit_effect_index;
	*/
};

struct PVGameData
{
	bool loaded;
	bool paused;
	bool byte2;
	bool byte3;
	uint8_t gap4[196];
	PVGamePvData pv_data;
	/*
	_BYTE gap2C8F8[1528];
	_BYTE byte2CEF0;
	_BYTE gap2CEF1[39];
	const char char2CF18;
	_BYTE gap2CF19[23];
	_QWORD qword2CF30;
	const char char2CF38;
	_BYTE gap2CF39[23];
	_QWORD qword2CF50;
	const char char2CF58;
	_BYTE gap2CF59[23];
	_QWORD qword2CF70;
	const char char2CF78;
	_BYTE gap2CF79[23];
	_QWORD qword2CF90;
	const char char2CF98;
	_BYTE gap2CF99[23];
	_QWORD qword2CFB0;
	const char char2CFB8;
	_BYTE gap2CFB9[23];
	_QWORD qword2CFD0;
	const char char2CFD8;
	_BYTE gap2CFD9[23];
	_QWORD qword2CFF0;
	const char char2CFF8;
	_BYTE gap2CFF9[23];
	_QWORD qword2D010;
	_BYTE gap2D018[357];
	_BYTE byte2D17D;
	_BYTE gap2D17E[182];
	int life;
	_DWORD dword2D238;
	_BYTE gap2D23C[4];
	_BYTE byte2D240;
	_BYTE gap2D241[7];
	unsigned int unsigned_int2D248;
	_DWORD dword2D24C;
	unsigned int unsigned_int2D250;
	_DWORD dword2D254;
	_BYTE gap2D258[64];
	float float2D298;
	_BYTE gap2D29C[12];
	int int2D2A8;
	_BYTE gap2D2AC;
	_BYTE byte2D2AD;
	__declspec(align(4)) float float2D2B0;
	_WORD word2D2B4;
	_BYTE gap2D2B6[10];
	_DWORD dword2D2C0;
	_BYTE gap2D2C4[8];
	int32_t reference_score;
	int32_t reference_score_with_life;
	int32_t dword2D2D4;
	std::vector__int32_t target_reference_scores;
	_BYTE gap2D2F0[8];
	_QWORD qword2D2F8;
	_BYTE gap2D300[29];
	_BYTE byte2D31D;
	_BYTE gap2D31E;
	_BYTE byte2D31F;
	int64_t challenge_time_start;
	int64_t challenge_time_end;
	int64_t pv_end_time;
	float pv_end_time_sec;
	float float2D33C;
	_BYTE gap2D340[4];
	float float2D344;
	_QWORD qword2D348;
	float float2D350;
	float float2D354;
	_DWORD dword2D358;
	_DWORD dword2D35C;
	_BYTE gap2D360[16];
	_DWORD dword2D370;
	_BYTE byte2D374;
	_BYTE byte2D375;
	_BYTE gap2D376[30];
	int32_t current_frame;
	_DWORD dword2D398;
	_DWORD dword2D39C;
	_DWORD dword2D3A0;
	_BYTE gap2D3A4[5];
	_BYTE byte2D3A9;
	_BYTE gap2D3AA;
	_BYTE byte2D3AB;
	_BYTE gap2D3AC[2];
	_BYTE byte2D3AE;
	_BYTE byte2D3AF;
	_BYTE gap2D3B0[16];
	_DWORD dword2D3C0;
	_BYTE gap2D3C4[16];
	_BYTE byte2D3D4;
	_BYTE gap2D3D5[19];
	_DWORD dword2D3E8;
	_BYTE gap2D3EC[2020];
	_QWORD qword2DBD0;
	_QWORD qword2DBD8;
	float float2DBE0;
	_DWORD dword2DBE4;
	_BYTE byte2DBE8;
	_BYTE byte2DBE9;
	_BYTE byte2DBEA;
	__declspec(align(2)) float float2DBEC;
	_BYTE byte2DBF0;
	_BYTE gap2DBF1[11];
	float float2DBFC;
	_BYTE gap2DC00[684];
	__int64 int642DEAC;
	*/
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

//inline FUNCTION_PTR(bool, __fastcall, PlaySoundEffect2, 0x1405AAFB0, int32_t a1, const char* name);
//inline FUNCTION_PTR(void, PlaySoundEffect, 0x1405AA540, const char* name, float volume);

inline FUNCTION_PTR(PVGameData*, __fastcall, GetPVGameData, 0x140266720);
inline FUNCTION_PTR(bool, __fastcall, IsInSongResults, 0x1401E8090);

inline bool ShouldUpdateTargets()
{
	PVGameData* pv_game = GetPVGameData();
	if (pv_game != nullptr)
		return !IsInSongResults() && !pv_game->paused && !pv_game->byte2;

	return false;
}
