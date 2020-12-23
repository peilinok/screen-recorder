#include "sws_helper.h"

#include "error_define.h"

namespace am {

	sws_helper::sws_helper()
	{
		_inited = false;

		_frame = NULL;

		_buffer = NULL;

		_ctx = NULL;
	}


	sws_helper::~sws_helper()
	{
		cleanup();
	}

	int sws_helper::init(AVPixelFormat src_fmt, int src_width, int src_height, AVPixelFormat dst_fmt, int dst_width, int dst_height)
	{
		if (_inited)
			return AE_NO;

		_ctx = sws_getContext(
			src_width,
			src_height,
			src_fmt,
			dst_width,
			dst_height,
			dst_fmt,
			SWS_BICUBIC,
			NULL, NULL, NULL
		);

		if (!_ctx) {
			return AE_FFMPEG_NEW_SWSCALE_FAILED;
		}

		_buffer_size = av_image_get_buffer_size(dst_fmt, dst_width, dst_height, 1);
		_buffer = new uint8_t[_buffer_size];

		_frame = av_frame_alloc();

		av_image_fill_arrays(_frame->data, _frame->linesize, _buffer, dst_fmt, dst_width, dst_height, 1);

		_inited = true;

		return AE_NO;
	}

	int sws_helper::convert(const AVFrame *frame, uint8_t ** out_data, int * len)
	{
		int error = AE_NO;
		if (!_inited || !_ctx || !_buffer)
			return AE_NEED_INIT;

		int ret = sws_scale(
			_ctx,
			(const uint8_t *const *)frame->data,
			frame->linesize,
			0, frame->height,
			_frame->data, _frame->linesize
		);

		*out_data = _buffer;
		*len = _buffer_size;

		return error;
	}

	void sws_helper::cleanup()
	{
		_inited = false;

		if (_ctx)
			sws_freeContext(_ctx);

		_ctx = NULL;

		if (_frame)
			av_frame_free(&_frame);

		_frame = NULL;

		if (_buffer)
			delete[] _buffer;

		_buffer = NULL;
	}

}