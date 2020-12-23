#include "record_audio.h"

namespace am {
	record_audio::record_audio()
	{
		_running = false;
		_inited = false;
		_paused = false;

		_sample_rate = 48000;
		_bit_rate = 3072000;
		_channel_num = 2;
		_channel_layout = av_get_default_channel_layout(_channel_num);
		_bit_per_sample = _bit_rate / _sample_rate / _channel_num;
		_fmt = AV_SAMPLE_FMT_FLT;
		_on_data = nullptr;
		_on_error = nullptr;

		_device_name = "";
		_device_id = "";
		_is_input = false;
	}

	record_audio::~record_audio()
	{

	}
}
