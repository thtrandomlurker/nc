#include <stdarg.h>
#include <string.h>
#include "util.h"

constexpr size_t MaxBufferSize = 2048;

std::string util::Format(const char* fmt, ...)
{
	char buffer[MaxBufferSize] = { 0 };
	
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf_s(buffer, fmt, argptr);
	va_end(argptr);

	return std::string(buffer);
}

std::string util::ChangeExtension(std::string_view view, std::string_view ext)
{
	if (view.empty())
		return "";

	std::string_view path_without_ext = view;
	for (auto it = view.rbegin(); it != view.rend(); it++)
	{
		if (*it == '.')
		{
			path_without_ext = view.substr(0, view.size() - (it - view.rbegin()) - 1);
			break;
		}
	}

	return std::string(path_without_ext) + std::string(ext);
}

bool util::StartsWith(std::string_view str, std::string_view prefix)
{
	if (prefix.size() > str.size())
		return false;

	return strncmp(str.data(), prefix.data(), prefix.size()) == 0;
}

bool util::EndsWith(std::string_view str, std::string_view suffix)
{
	if (suffix.size() > str.size())
		return false;

	return strncmp(str.substr(str.size() - suffix.size()).data(), suffix.data(), suffix.size()) == 0;
}