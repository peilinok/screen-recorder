#ifndef RECORDER_EXPORT
#define RECORDER_EXPORT

#include <stdint.h>

#ifdef AMRECORDER_IMPORT
#define AMRECORDER_API extern "C"  __declspec(dllimport)
#else
#define AMRECORDER_API extern "C"  __declspec(dllexport)
#endif


#pragma pack(push,1)
typedef struct {
	char id[260];
	char name[260];
	uint8_t is_default;
}AMRECORDER_DEVICE;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
	int v_left;
	int v_top;
	int v_width;
	int v_height;
	int v_qb;
	int v_bit_rate;
	int v_frame_rate;
	int v_enc_id;

	char output[260];

	AMRECORDER_DEVICE v_device;
	AMRECORDER_DEVICE a_mic;
	AMRECORDER_DEVICE a_speaker;
}AMRECORDER_SETTING;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
	int id;
	char name[260];
}AMRECORDER_ENCODERS;
#pragma pack(pop)


typedef void( *AMRECORDER_FUNC_DURATION)(uint64_t duration);

typedef void( *AMRECORDER_FUNC_ERROR)(int error);

typedef void( *AMRECORDER_FUNC_DEVICE_CHANGE)(int type);//0 video 1 speaker 2 mic

typedef void(*AMRECORDER_FUNC_PREVIEW_YUV)(
	const unsigned char *data,
	unsigned int size,
	int width,
	int height,
	int type
	); // 0 420 1 444

typedef void( *AMRECORDER_FUNC_PREVIEW_AUDIO)();

/**
* Remux progress callback function
* @param[in] path       source file path
* @param[in] progress   remuxing progress in total
* @param[in] total      always will be 100
*/
typedef void(*AMRECORDER_FUNC_REMUX_PROGRESS)(const char *path, int progress, int total);

/**
* Remux state callback function
* @param[in] path    source file path
* @param[in] state   0 for unremuxing,1 for remuxing
* @param[in] error   0 for succed,otherwhise error code
*/
typedef void(*AMRECORDER_FUNC_REMUX_STATE)(const char *path, int state, int error);

#pragma pack(push,1)
typedef struct {
	AMRECORDER_FUNC_DURATION func_duration;
	AMRECORDER_FUNC_ERROR func_error;
	AMRECORDER_FUNC_DEVICE_CHANGE func_device_change;
	AMRECORDER_FUNC_PREVIEW_YUV func_preview_yuv;
	AMRECORDER_FUNC_PREVIEW_AUDIO func_preview_audio;
}AMRECORDER_CALLBACK;
#pragma pack(pop)

AMRECORDER_API const char * recorder_err2str(int error);

AMRECORDER_API int recorder_init(const AMRECORDER_SETTING &setting, const AMRECORDER_CALLBACK &callbacks);

AMRECORDER_API void recorder_release();


AMRECORDER_API int recorder_start();

AMRECORDER_API void recorder_stop();

AMRECORDER_API int recorder_pause();

AMRECORDER_API int recorder_resume();

AMRECORDER_API int recorder_get_speakers(AMRECORDER_DEVICE **devices);

AMRECORDER_API int recorder_get_mics(AMRECORDER_DEVICE **devices);

AMRECORDER_API int recorder_get_cameras(AMRECORDER_DEVICE **devices);

AMRECORDER_API int recorder_get_vencoders(AMRECORDER_ENCODERS **encoders);

/**
* Recorder create a remux job
* @param[in] src    source file path
* @param[in] dst   0 for unremuxing,1 for remuxing
* @param[in] func_progress   0 for succed,otherwhise error code
* @param[in] func_state   0 for succed,otherwhise error code
*/
AMRECORDER_API int recorder_remux(
	const char *src, const char *dst, 
	AMRECORDER_FUNC_REMUX_PROGRESS func_progress, 
	AMRECORDER_FUNC_REMUX_STATE func_state);

#endif