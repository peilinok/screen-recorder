#ifndef RECORD_AUDIO_WASAPI
#define RECORD_AUDIO_WASAPI

#include "record_audio.h"

#ifdef _WIN32


#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

#endif // _WIN32

namespace am {
	class record_audio_wasapi:public record_audio
	{
	public:
		record_audio_wasapi();
		~record_audio_wasapi();

		virtual int init(const std::string &device_name);

		virtual int start();

		virtual int pause();

		virtual int resume();

		virtual int stop();

		virtual const AVRational & get_time_base();

		virtual int64_t get_start_time();

	private:
		void record_func();

		bool adjust_format_2_16bits(WAVEFORMATEX *pwfx);

		void clean_wasapi();

	private:
		WAVEFORMATEX *_wfex;

		IMMDeviceEnumerator *_enumerator;

		IMMDevice *_device;

		IAudioClient *_client;

		IAudioCaptureClient *_capture;

		HANDLE _ready_event;

		bool _co_inited;
	};
}

#endif // !RECORD_AUDIO_WASAPI

