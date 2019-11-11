#ifndef RECORDER_EXPORT
#define RECORDER_EXPORT

#include <stdint.h>


#ifdef AMRECORDER_IMPORT
#define AMRECORDER_API extern "C"  __declspec(dllimport)
#else
#define AMRECORDER_API extern "C"  __declspec(dllexport)
#endif


typedef struct {
	
}AMRECORDER_SETTING;

typedef struct {
	
}AMRECORDER_CALLBACK;

typedef struct {
	uint8_t id[260];
	uint8_t name[260];
	uint8_t is_default;
}AMRECORDER_DEVICE;


AMRECORDER_API int recorder_init(const AMRECORDER_SETTING &setting, const AMRECORDER_CALLBACK &callbacks);

AMRECORDER_API void recorder_release();


AMRECORDER_API int recorder_start();

AMRECORDER_API void recorder_stop();

AMRECORDER_API int recorder_pause();

AMRECORDER_API int recorder_resume();

AMRECORDER_API int recorder_get_speakers(AMRECORDER_DEVICE **devices);

AMRECORDER_API int recorder_get_mics(AMRECORDER_DEVICE **devices);

AMRECORDER_API int recorder_get_cameras(AMRECORDER_DEVICE **devices);

#endif