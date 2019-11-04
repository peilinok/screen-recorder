#ifndef ENCODER_AAC
#define ENCODER_AAC

#include <atomic>
#include <thread>
#include <functional>

extern "C" {
#include <libavformat\avformat.h>
#include <libavcodec\avcodec.h>
}

#include "ring_buffer.h"

//#define SAVE_AAC

namespace am {
	typedef std::function<void(const uint8_t*, int)> cb_aac_data;
	typedef std::function<void(int)> cb_aac_error;

	class encoder_aac {
	public:
		encoder_aac();
		~encoder_aac();

		int init(
			int nb_channels,
			int nb_samples,
			int sample_rate,
			AVSampleFormat fmt,
			int bit_rate
		);

		int start();

		void stop();
		
		int put(const uint8_t *data,int data_len);

		inline void registe_cb(
			cb_aac_data on_data,
			cb_aac_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}

	protected:
		void encode_loop();

		void cleanup();

	private:
		cb_aac_data _on_data;
		cb_aac_error _on_error;

		ring_buffer *_ring_buffer;

		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		AVCodec *_encoder;
		AVCodecContext *_encoder_ctx;
		AVFrame *_frame;
		uint8_t *_buff;
		int _buff_size;

#ifdef SAVE_AAC
		AVIOContext *_aac_io_ctx;
		AVStream *_aac_stream;
		AVFormatContext *_aac_fmt_ctx;
#endif
	};
}



#endif
