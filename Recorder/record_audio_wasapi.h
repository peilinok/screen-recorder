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

		virtual int init(const std::string &device_name,
			const std::string &device_id,
			bool is_input);

		virtual int start();

		virtual int pause();

		virtual int resume();

		virtual int stop();

		virtual const AVRational & get_time_base();

		virtual int64_t get_start_time();

	private:
		int init_render();

		void render_func();

		void process_data(AVFrame *frame,uint8_t* data, uint32_t sample_count);

		bool do_record_input(AVFrame *frame);

		bool do_record_output(AVFrame *frame);

		void record_func_input();

		void record_func_output();

		bool adjust_format_2_16bits(WAVEFORMATEX *pwfx);

		void clean_wasapi();

	private:
		WAVEFORMATEX *_wfex;

		IMMDeviceEnumerator *_enumerator;

		IMMDevice *_device;

		IAudioClient *_capture_client;

		IAudioCaptureClient *_capture;

		IAudioRenderClient *_render;

		IAudioClient *_render_client;

		std::thread _render_thread;

		uint32_t _capture_sample_count;
		uint32_t _render_sample_count;

		HANDLE _ready_event;
		HANDLE _stop_event;
		HANDLE _render_event;

		bool _co_inited;

		bool _is_default;

		uint8_t *_silent_data;
		uint32_t _silent_sample_count;
		uint32_t _silent_data_size;

		//define time stamps here
		int64_t _start_time;
		int64_t _pre_pts;
	};
}

#endif // !RECORD_AUDIO_WASAPI

