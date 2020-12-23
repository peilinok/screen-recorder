
#define RAY_EXPORT

#include "ray_recorder.h"
#include "ray_remuxer.h"

#include "ray_dump.h"

#include "constants\ray_version.h"

bool APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		registerExceptionHandler();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

RAY_API void getVersion(uint32_t *major, 
	uint32_t *minor, uint32_t *patch, uint32_t *build)
{
	if (major) *major = VER_MAJOR;

	if (minor) *minor = VER_MINOR;

	if (patch) *patch = VER_PATCH;

	if (build) *build = VER_BUILD;
}


RAY_API ray::recorder::IRecorder *createRecorder() {
	return ray::recorder::Recorder::getInstance();
}

RAY_API ray::remuxer::IRemuxer *createRemuxer() {
	return ray::remuxer::Remuxer::getInstance();
}

