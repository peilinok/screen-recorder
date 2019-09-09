#ifndef RECORD_AUDIO
#define RECORD_AUDIO

#include "record_audio_define.h"

#include <atomic>
#include <thread>
#include <functional>

namespace am {
	typedef std::function<void(const uint8_t*, int)> cb_audio_data;
	typedef std::function<void(int)> cb_audio_error;

	class record_audio
	{
	public:
		record_audio();
		virtual ~record_audio();

		virtual int init(
			const std::string &device_name,
			const RECORD_AUDIO_FORMAT fmt,
			const int sample_rate,
			const int bit_rate) = 0;

		virtual int start() = 0;
		
		virtual int pause() = 0;
		
		virtual int resume() = 0;
		
		virtual int stop() = 0;

	public:
		inline bool is_recording() { return _running; }
		
		inline int get_sample_rate() { return _sample_rate; }
		
		inline int get_bit_rate() { return _bit_rate; }
		
		inline RECORD_AUDIO_FORMAT get_fmt() { return _fmt; }

		inline std::string get_device_name() { return _device_name; }

		inline void registe_cb(
			cb_audio_data on_data,
			cb_audio_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}

	protected:
		std::atomic_bool _running;
		std::atomic_bool _inited;

		std::thread _thread;

		int _sample_rate;
		
		int _bit_rate;

		int _channel_num;

		int _bit_per_sample;
		
		RECORD_AUDIO_FORMAT _fmt;
		
		std::string _device_name;

		cb_audio_error _on_error;
		
		cb_audio_data _on_data;
	};
}

#endif // !RECORD_AUDIO