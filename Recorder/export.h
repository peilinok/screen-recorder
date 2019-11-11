#ifndef RECORDER_EXPORT
#define RECORDER_EXPORT

#include <stdint.h>


#ifdef AMRECORDER_IMPORT
#define AMRECORDER_API extern "C"  __declspec(dllimport)
#else
#define AMRECORDER_API extern "C"  __declspec(dllexport)
#endif

typedef struct {
	char id[260];
	char name[260];
	uint8_t is_default;
}AMRECORDER_DEVICE;

typedef struct {
	int v_left;
	int v_top;
	int v_width;
	int v_height;
	int v_qb;
	int v_bit_rate;
	int v_frame_rate;

	char output[260];

	AMRECORDER_DEVICE v_device;
	AMRECORDER_DEVICE a_mic;
	AMRECORDER_DEVICE a_speaker;
}AMRECORDER_SETTING;

typedef void(*AMRECORDER_FUNC_DURATION)(uint64_t duration);

typedef void(*AMRECORDER_FUNC_ERROR)(int error);

typedef void(*AMRECORDER_FUNC_DEVICE_CHANGE)(int type);//0 video 1 speaker 2 mic

typedef struct {
	AMRECORDER_FUNC_DURATION func_duration;
	AMRECORDER_FUNC_ERROR func_error;
	AMRECORDER_FUNC_DEVICE_CHANGE func_device_change;
}AMRECORDER_CALLBACK;


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