#ifndef ENCODER_AAC
#define ENCODER_AAC

#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "headers_ffmpeg.h"
#include "ring_buffer.h"

//#define SAVE_AAC

namespace am {
	typedef std::function<void(AVPacket *packet)> cb_aac_data;
	typedef std::function<void(int)> cb_aac_error;


	class encoder_aac {
	public:
		encoder_aac();
		~encoder_aac();

		int init(
			int nb_channels,
			int sample_rate,
			AVSampleFormat fmt,
			int bit_rate
		);

		int get_extradata_size();
		const uint8_t* get_extradata();

		int get_nb_samples();

		int start();

		void stop();
		
		int put(const uint8_t *data,int data_len,AVFrame *frame);

		inline void registe_cb(
			cb_aac_data on_data,
			cb_aac_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}

		const AVRational &get_time_base();

		AVCodecID get_codec_id();

	private:
		int encode(AVFrame *frame, AVPacket *packet);

		void encode_loop();

		void cleanup();

	private:
		cb_aac_data _on_data;
		cb_aac_error _on_error;

		ring_buffer<AVFrame> *_ring_buffer;

		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		AVCodec *_encoder;
		AVCodecContext *_encoder_ctx;
		AVFrame *_frame;
		uint8_t *_buff;
		int _buff_size;

		std::mutex _mutex;
		std::condition_variable _cond_var;
		bool _cond_notify;

#ifdef SAVE_AAC
		AVIOContext *_aac_io_ctx;
		AVStream *_aac_stream;
		AVFormatContext *_aac_fmt_ctx;
#endif
	};
}



#endif
