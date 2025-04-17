#pragma once

#include <vector>

// NOTE: Helper functions
template <typename T>
const T* FindWithID(const std::vector<T>& vec, int32_t id)
{
	for (const auto& data : vec)
		if (data.id == id)
			return &data;
	return nullptr;
}