#include "system_lib.h"

#include "log_helper.h"

namespace am {

	static char system_path[260] = { 0 };

	static bool get_system_path() {

		if (strlen(system_path) == 0) {
			UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
			if (!ret) {
				al_fatal("failed to get system directory :%lu", GetLastError());
				return false;
			}
		}

		return true;
	}

	HMODULE load_system_library(const char * name)
	{
		if (get_system_path() == false) return NULL;

		char base_path[MAX_PATH] = { 0 };
		strcpy(base_path, system_path);
		strcat(base_path, "\\");
		strcat(base_path, name);

		HMODULE module = GetModuleHandleA(base_path);
		if (module)
			return module;

		module = LoadLibraryA(base_path);
		if (!module) {
			al_error("failed load system library :%lu", GetLastError());
		}

		return module;
	}

	void free_system_library(HMODULE handle)
	{
		FreeModule(handle);
	}

}