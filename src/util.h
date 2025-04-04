#pragma once

#include <array>
#include <string>
#include <string_view>

namespace util
{
	std::string Format(const char* fmt, ...);
	std::string ChangeExtension(std::string_view view, std::string_view ext);
	
	template <size_t S>
	int32_t GetIndex(std::array<std::string_view, S> data, std::string_view search, int32_t default_value = -1)
	{
		for (size_t i = 0; i < S; i++)
			if (search.compare(data[i]) == 0)
				return i;

		return default_value;
	}
}