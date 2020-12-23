#include "encoder_video.h"

namespace am {

	encoder_video::encoder_video()
	{
		_inited = false;
		_running = false;

		_time_base = { 0,AV_TIME_BASE };

		_encoder_id = EID_VIDEO_X264;
		_encoder_type = ET_VIDEO_SOFT;

		_cond_notify = false;

		_ring_buffer = new ring_buffer<AVFrame>();
	}


	encoder_video::~encoder_video()
	{
		if(_ring_buffer)
			delete _ring_buffer;
	}

	int encoder_video::start()
	{
		int error = AE_NO;

		if (_running == true) {
			return error;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&encoder_video::encode_loop, this));

		return error;
	}

	void encoder_video::stop()
	{
		_running = false;

		_cond_notify = true;
		_cond_var.notify_all();

		if (_thread.joinable())
			_thread.join();
	}

	int encoder_video::put(const uint8_t * data, int data_len, AVFrame * frame)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		AVFrame frame_cp;
		memcpy(&frame_cp, frame, sizeof(AVFrame));

		_ring_buffer->put(data, data_len, frame_cp);

		_cond_notify = true;
		_cond_var.notify_all();
		return 0;
	}

}