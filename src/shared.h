#pragma once

#include <stdint.h>

enum ScoreMode : int32_t
{
	ScoreMode_Arcade   = 0,
	// ScoreMode_F2nd  = 1,
	ScoreMode_Franken  = 2,
	ScoreMode_Max
};

enum GameStyle : int32_t
{
	GameStyle_Arcade  = 0,
	GameStyle_Console = 1,
	GameStyle_Mixed   = 2,
	GameStyle_Max
};
