#pragma once

#include <array>
#include <string>
#include <string_view>

namespace util
{
	std::string Format(const char* fmt, ...);
	std::string ChangeExtension(std::string_view view, std::string_view ext);
	bool StartsWith(std::string_view str, std::string_view prefix);
	bool EndsWith(std::string_view str, std::string_view suffix);
	
	template <size_t S>
	int32_t GetIndex(std::array<std::string_view, S> data, std::string_view search, int32_t default_value = -1)
	{
		for (size_t i = 0; i < S; i++)
			if (search.compare(data[i]) == 0)
				return i;

		return default_value;
	}

	template <typename T>
	inline T Clamp(T value, T min, T max)
	{
		return value < min ? min : value > max ? max : value;
	}

	template <typename T>
	inline T Wrap(T value, T min, T max)
	{
		return value < min ? max : value > max ? min : value;
	}

	inline int32_t ColorF32I32(float r, float g, float b, float a)
	{
		return Clamp<int32_t>(r * 255, 0, 255) |
			(Clamp<int32_t>(g * 255.0f, 0, 255) << 8) |
			(Clamp<int32_t>(b * 255.0f, 0, 255) << 16) |
			(Clamp<int32_t>(a * 255.0f, 0, 255) << 24);
	}
}