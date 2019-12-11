#ifndef RECORD_AUDIO_WASAPI
#define RECORD_AUDIO_WASAPI

#include "record_audio.h"

#ifdef _WIN32

#include "mmdevice_define.h"

#include <windows.h>
#include <propsys.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

#endif // _WIN32

namespace am {
	class record_audio_wasapi:public record_audio
	{
	public:
		record_audio_wasapi();
		~record_audio_wasapi();

		virtual int init(const std::string &device_name,bool is_input);

		virtual int start();

		virtual int pause();

		virtual int resume();

		virtual int stop();

		virtual const AVRational & get_time_base();

		virtual int64_t get_start_time();

	private:
		void init_render();

		bool do_record(AVFrame *frame);

		void record_func();

		bool adjust_format_2_16bits(WAVEFORMATEX *pwfx);

		void clean_wasapi();

	private:
		WAVEFORMATEX *_wfex;

		IMMDeviceEnumerator *_enumerator;

		IMMDevice *_device;

		IAudioClient *_client;

		IAudioCaptureClient *_capture;

		IAudioRenderClient *_render;

		HANDLE _ready_event;
		HANDLE _stop_event;

		bool _co_inited;

		//define time stamps here
		int64_t _start_time;
		int64_t _pre_pts;
	};
}

#endif // !RECORD_AUDIO_WASAPI

