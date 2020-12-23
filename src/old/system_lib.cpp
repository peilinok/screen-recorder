#include "system_lib.h"

#include "utils\ray_log.h"

namespace am {

	static char system_path[260] = { 0 };

	static bool get_system_path() {

		if (strlen(system_path) == 0) {
			UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
			if (!ret) {
				LOG(FATAL) << "failed to get system directory: " << GetLastError();
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
			LOG(ERROR) << "failed load system library: " << GetLastError();
		}

		return module;
	}

	void free_system_library(HMODULE handle)
	{
		FreeModule(handle);
	}

}