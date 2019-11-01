#ifndef ENCODER_264
#define ENCODER_264

#include <atomic>
#include <thread>
#include <functional>

extern "C" {
#include <libavformat\avformat.h>
#include <libavcodec\avcodec.h>
}

namespace am {
	typedef std::function<void(const uint8_t*, int)> cb_264_data;
	typedef std::function<void(int)> cb_264_error;

	class encoder_264 
	{
	public:
		encoder_264();
		~encoder_264();

		int init(int pic_width, int pic_height, int frame_rate, int *buff_size, int gop_size = 28);

		int encode(const uint8_t *src, int src_len,unsigned char *dst,int dst_len,int *got_pic);

		void release();

	protected:
		void cleanup();

	private:
		cb_264_data _on_data;
		cb_264_error _on_error;

		std::atomic_bool _inited;
		AVCodec *_encoder;
		AVCodecContext *_encoder_ctx;
		AVFrame *_frame;
		uint8_t *_buff;
		int _buff_size;
		int _y_size;
	};
}


#endif
