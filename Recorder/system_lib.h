#pragma once

#include <Windows.h>

namespace am{
	HMODULE load_system_library(const char *name);
	
	void free_system_library(HMODULE handle);

}