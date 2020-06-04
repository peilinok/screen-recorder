#ifndef ENCODER_VIDEO
#define ENCODER_VIDEO

#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "headers_ffmpeg.h"
#include "ring_buffer.h"

#include "encoder_video_define.h"

namespace am {

	typedef std::function<void(AVPacket *packet)> cb_264_data;
	typedef std::function<void(int)> cb_264_error;

	class encoder_video
	{
	public:
		encoder_video();
		virtual ~encoder_video();

		virtual int init(
			int pic_width, 
			int pic_height,
			int frame_rate,
			int bit_rate, 
			int qb, 
			int key_pic_sec = 2) = 0;

		virtual int get_extradata_size() = 0;
		virtual const uint8_t* get_extradata() = 0;

		inline void registe_cb(
			cb_264_data on_data,
			cb_264_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}

		inline const AVRational &get_time_base() {
			return _time_base;
		};

		virtual int start();

		virtual void stop();

		virtual int put(const uint8_t *data, int data_len, AVFrame *frame);

		virtual AVCodecID get_codec_id() = 0;

	protected:
		virtual void cleanup() = 0;
		virtual void encode_loop() = 0;

	protected:
		ENCODER_VIDEO_ID _encoder_id;
		ENCODER_VIDEO_TYPES _encoder_type;

		cb_264_data _on_data;
		cb_264_error _on_error;

		ring_buffer<AVFrame> *_ring_buffer;

		AVRational _time_base;

		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		std::mutex _mutex;
		std::condition_variable _cond_var;
		bool _cond_notify;
	};

}

#endif // !ENCODER_VIDEO