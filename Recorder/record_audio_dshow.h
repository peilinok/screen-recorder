#pragma once

#include "record_audio.h"

extern "C" {
#include <libavformat\avformat.h>
#include <libavdevice\avdevice.h>
#include <libavcodec\avcodec.h>
}

namespace am {

	class record_audio_dshow :public record_audio
	{
	public:
		record_audio_dshow();
		~record_audio_dshow();

		virtual int init(const std::string &device_name);

		virtual int start();

		virtual int pause();

		virtual int resume();

		virtual int stop();

	private:
		void record_loop();
		void cleanup();
	private:
		AVFormatContext *_fmt_ctx;
		AVInputFormat *_input_fmt;
		AVCodecContext *_codec_ctx;
		AVCodec *_codec;

		int _stream_index;
	};

}