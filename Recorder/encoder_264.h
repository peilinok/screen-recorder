#ifndef ENCODER_264
#define ENCODER_264

#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "headers_ffmpeg.h"
#include "ring_buffer.h"

namespace am {
	typedef std::function<void(const AVPacket *packet)> cb_264_data;
	typedef std::function<void(int)> cb_264_error;

	class encoder_264 
	{
	public:
		encoder_264();
		~encoder_264();

		int init(int pic_width, int pic_height, int frame_rate, int bit_rate, int qb, int gop_size = 40);

		int get_extradata_size();
		const uint8_t* get_extradata();

		inline void registe_cb(
			cb_264_data on_data,
			cb_264_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}

		const AVRational &get_time_base();

		int start();

		void stop();

		int put(const uint8_t *data, int data_len, AVFrame *frame);

	protected:
		void cleanup();

	private:
		int encode(AVFrame *frame, AVPacket *packet);
		void encode_loop();

	private:
		cb_264_data _on_data;
		cb_264_error _on_error;

		ring_buffer<AVFrame> *_ring_buffer;

		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		AVCodec *_encoder;
		AVCodecContext *_encoder_ctx;
		AVFrame *_frame;
		uint8_t *_buff;
		int _buff_size;
		int _y_size;

		std::mutex _mutex;
		std::condition_variable _cond_var;
		bool _cond_notify;
	};
}


#endif
