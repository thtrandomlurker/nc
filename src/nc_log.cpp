#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "nc_log.h"

void nc::Print(const char* const fmt, ...)
{
	if (GetConsoleWindow() != NULL)
	{
		// NOTE: Print plugin tag
		printf("[NewClassics] ");

		// NOTE: Print passed information
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}

	// TODO: Add file logging (?)
}