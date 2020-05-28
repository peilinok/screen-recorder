#ifndef RECORD_DESKTOP_FFMPEG_GDI
#define RECORD_DESKTOP_FFMPEG_GDI

#include "record_desktop.h"

namespace am {

	class record_desktop_ffmpeg_gdi :public record_desktop
	{
	public:
		record_desktop_ffmpeg_gdi();
		~record_desktop_ffmpeg_gdi();

		virtual int init(
			const RECORD_DESKTOP_RECT &rect,
			const int fps);
		
		virtual int start();
		virtual int pause();
		virtual int resume();
		virtual int stop();

	protected:
		virtual void clean_up();

	private:
		int decode(AVFrame *frame, AVPacket *packet);

		void record_func();

		int _stream_index;
		AVFormatContext *_fmt_ctx;
		AVInputFormat *_input_fmt;
		AVCodecContext *_codec_ctx;
		AVCodec *_codec;
	};

}
#endif