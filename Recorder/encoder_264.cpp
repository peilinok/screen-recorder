#include "encoder_264.h"

#include "log_helper.h"
#include "error_define.h"

namespace am {

	encoder_264::encoder_264()
	{
		av_register_all();

		_inited = false;
		_running = false;

		_encoder = NULL;
		_encoder_ctx = NULL;
		_frame = NULL;
		_buff = NULL;
		_buff_size = 0;
		_y_size = 0;

		_ring_buffer = new ring_buffer();
	}

	encoder_264::~encoder_264()
	{
		stop();

		cleanup();

		delete _ring_buffer;
	}

	int encoder_264::init(int pic_width, int pic_height, int frame_rate, int *buff_size, int gop_size)
	{
		if (_inited == true)
			return AE_NO;

		int err = AE_NO;
		int ret = 0;

		AVDictionary *options = 0;

		av_dict_set(&options, "preset", "superfast", 0);
		av_dict_set(&options, "tune", "zerolatency", 0);

		do {
			_encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!_encoder) {
				err = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}

			_encoder_ctx = avcodec_alloc_context3(_encoder);
			if (!_encoder_ctx) {
				err = AE_FFMPEG_ALLOC_CONTEXT_FAILED;
				break;
			}

			_encoder_ctx->codec_id = AV_CODEC_ID_H264;
			_encoder_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
			_encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			_encoder_ctx->width = pic_width;
			_encoder_ctx->height = pic_height;
			_encoder_ctx->time_base.num = 1;
			_encoder_ctx->time_base.den = frame_rate;
			_encoder_ctx->bit_rate = 4000000;
			_encoder_ctx->gop_size = gop_size;
			_encoder_ctx->qmin = 10;
			_encoder_ctx->qmax = 51;
			//_encoder_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			_encoder = avcodec_find_encoder(_encoder_ctx->codec_id);
			if (!_encoder) {
				err = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}

			ret = avcodec_open2(_encoder_ctx, _encoder, &options);
			if (ret != 0) {
				err = AE_FFMPEG_OPEN_CODEC_FAILED;
				break;
			}

			_frame = av_frame_alloc();

			_buff_size = avpicture_get_size(_encoder_ctx->pix_fmt, _encoder_ctx->width, _encoder_ctx->height);
			
			//may no need this
			_buff = (uint8_t*)av_malloc(_buff_size);
			if (!_buff) {
				break;
			}

			avpicture_fill((AVPicture*)_frame, _buff, _encoder_ctx->pix_fmt, _encoder_ctx->width, _encoder_ctx->height);
			//may no need above

			_frame->format = _encoder_ctx->pix_fmt;
			_frame->width = _encoder_ctx->width;
			_frame->height = _encoder_ctx->height;

			_y_size = _encoder_ctx->width * _encoder_ctx->height;
			if(buff_size)
				*buff_size = _buff_size;

			_inited = true;
		} while (0);

		if (err != AE_NO) {
			al_debug("%s,error:%d %ld", err2str(err), ret, GetLastError());
			cleanup();
		}

		if(options)
			av_dict_free(&options);

		return err;
	}

	int encoder_264::start()
	{
		int error = AE_NO;

		if (_running == true) {
			return error;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&encoder_264::encode_loop, this));

		return error;
	}

	void encoder_264::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();

	}

	int encoder_264::put(const uint8_t * data, int data_len)
	{
		_ring_buffer->put(data, data_len);
		return 0;
	}

	int encoder_264::encode(const uint8_t *src, int src_len, unsigned char * dst, int dst_len, int * got_pic)
	{
		if (_inited == false)
			return AE_NEED_INIT;

		int err = AE_NO;

		AVPacket packet;
		av_new_packet(&packet, _buff_size);

		memcpy_s(_buff, _buff_size, src, src_len);

		_frame->data[0] = _buff;
		_frame->data[1] = _buff + _y_size;
		_frame->data[2] = _buff + _y_size * 5 / 4;

		int ret = avcodec_encode_video2(_encoder_ctx, &packet, _frame, got_pic);
		if (ret != 0) {
			err = AE_FFMPEG_ENCODE_FRAME_FAILED;
		}
		else if(*got_pic){
			*got_pic = packet.size;
			memcpy(dst, packet.data, min(packet.size, dst_len));
		}

		av_free_packet(&packet);

		return err;
	}

	void encoder_264::cleanup()
	{
		if (_frame)
			av_free(_frame);
		_frame = NULL;

		if (_buff)
			av_free(_buff);

		_buff = NULL;

		if (_encoder)
			avcodec_close(_encoder_ctx);

		_encoder = NULL;

		if (_encoder_ctx)
			avcodec_free_context(&_encoder_ctx);

		_encoder_ctx = NULL;
	}

	void encoder_264::encode_loop()
	{
		int len, ret, got_pic = 0;

		AVPacket *packet = av_packet_alloc();

		while (_running)
		{
			len = _ring_buffer->get(_buff, _buff_size);

			_frame->data[0] = _buff;
			_frame->data[1] = _buff + _y_size;
			_frame->data[2] = _buff + _y_size * 5 / 4;

			if (len) {
				ret = avcodec_send_frame(_encoder_ctx, _frame);
				if (ret < 0) {
					if (_on_error) _on_error(AE_FFMPEG_ENCODE_FRAME_FAILED);
					al_fatal("encode yuv frame failed:%d", ret);

					continue;
				}

				while (ret >= 0) {
					ret = avcodec_receive_packet(_encoder_ctx, packet);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						break;
					}

					if (ret < 0) {
						if (_on_error) _on_error(AE_FFMPEG_READ_PACKET_FAILED);

						al_fatal("read aac packet failed:%d", ret);
					}

					if (_on_data) _on_data(packet->data, packet->size,packet->flags == AV_PKT_FLAG_KEY);

					av_packet_unref(packet);
				}
			}
			else
				_sleep(10);//should use condition_variable instead
		}

		av_free_packet(packet);
	}

}