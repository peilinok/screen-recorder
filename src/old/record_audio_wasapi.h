#ifndef RECORD_AUDIO_WASAPI
#define RECORD_AUDIO_WASAPI

#include "record_audio.h"

#ifdef _WIN32

#include "headers_mmdevice.h"

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

		virtual const AVRational get_time_base();

		virtual int64_t get_start_time();

	private:
		int64_t convert_layout(DWORD layout, WORD channels);

		void init_format(WAVEFORMATEX *wfex);

		int init_render();

		void render_loop();

		void process_data(AVFrame *frame, uint8_t* data, uint32_t sample_count, uint64_t device_ts);

		int do_record(AVFrame *frame);

		void record_loop();

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

		bool _use_device_ts;

		//define time stamps here
		int64_t _start_time;
	};
}

#endif // !RECORD_AUDIO_WASAPI

