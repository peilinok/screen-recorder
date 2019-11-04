#ifndef ENCODER_264
#define ENCODER_264

#include <atomic>
#include <thread>
#include <functional>

extern "C" {
#include <libavformat\avformat.h>
#include <libavcodec\avcodec.h>
}

#include "ring_buffer.h"

namespace am {
	typedef std::function<void(const uint8_t*, int)> cb_264_data;
	typedef std::function<void(int)> cb_264_error;

	class encoder_264 
	{
	public:
		encoder_264();
		~encoder_264();

		int init(int pic_width, int pic_height, int frame_rate, int *buff_size, int gop_size = 28);

		inline void registe_cb(
			cb_264_data on_data,
			cb_264_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}

		int start();

		void stop();

		int put(const uint8_t *data,int data_len);

		int encode(const uint8_t *src, int src_len,unsigned char *dst,int dst_len,int *got_pic);

	protected:
		void cleanup();

	private:
		void encode_loop();

	private:
		cb_264_data _on_data;
		cb_264_error _on_error;

		ring_buffer *_ring_buffer;

		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		AVCodec *_encoder;
		AVCodecContext *_encoder_ctx;
		AVFrame *_frame;
		uint8_t *_buff;
		int _buff_size;
		int _y_size;
	};
}


#endif
