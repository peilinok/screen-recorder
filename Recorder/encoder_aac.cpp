#include "encoder_aac.h"

#include "error_define.h"
#include "log_helper.h"

namespace am {

	encoder_aac::encoder_aac()
	{
		_ring_buffer = new ring_buffer(1024 * 1024 * 10);

		_inited = false;
		_running = false;

		_encoder = NULL;
		_encoder_ctx = NULL;
		_frame = NULL;
		_buff = NULL;
		_buff_size = 0;

#ifdef SAVE_AAC
		_aac_io_ctx = NULL;
		_aac_stream = NULL;
		_aac_fmt_ctx = NULL;
#endif
	}

	encoder_aac::~encoder_aac()
	{
		stop();

		cleanup();

		delete _ring_buffer;
	}

	int encoder_aac::init(
		int nb_channels, 
		int nb_samples,
		int sample_rate, 
		AVSampleFormat fmt,
		int bit_rate)
	{
		int err = AE_NO;
		int ret = 0;

		if (_inited == true)
			return err;

		do {
			_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
			if (!_encoder) {
				err = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}

			_encoder_ctx = avcodec_alloc_context3(_encoder);
			if (!_encoder_ctx) {
				err = AE_FFMPEG_ALLOC_CONTEXT_FAILED;
				break;
			}

			_encoder_ctx->channels = nb_channels;
			_encoder_ctx->channel_layout = av_get_default_channel_layout(nb_channels);
			_encoder_ctx->sample_rate = sample_rate;
			_encoder_ctx->sample_fmt = fmt;
			_encoder_ctx->bit_rate = bit_rate;

			_encoder_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
			_encoder_ctx->time_base.den = sample_rate;
			_encoder_ctx->time_base.num = 1;

			ret = avcodec_open2(_encoder_ctx, _encoder, NULL);
			if (ret < 0) {
				err = AE_FFMPEG_OPEN_CODEC_FAILED;
				break;
			}

			_buff_size = av_samples_get_buffer_size(NULL, nb_channels, nb_samples, fmt, 0);
			_buff = (uint8_t*)av_malloc(_buff_size);

			_frame = av_frame_alloc();
			if (!_frame) {
				err = AE_FFMPEG_ALLOC_FRAME_FAILED;
				break;
			}

			_frame->nb_samples = nb_samples;
			_frame->channel_layout = av_get_default_channel_layout(nb_channels);
			_frame->format = fmt;
			_frame->sample_rate = sample_rate;

			ret = avcodec_fill_audio_frame(_frame, nb_channels, fmt, _buff, _buff_size, 0);

#ifdef SAVE_AAC
			ret = avio_open(&_aac_io_ctx, "save.aac", AVIO_FLAG_READ_WRITE);
			if (ret < 0) {
				err = AE_FFMPEG_OPEN_IO_FAILED;
				break;
			}

			_aac_fmt_ctx = avformat_alloc_context();
			if (!_aac_fmt_ctx) {
				err = AE_FFMPEG_ALLOC_CONTEXT_FAILED;
				break;
			}

			_aac_fmt_ctx->pb = _aac_io_ctx;
			_aac_fmt_ctx->oformat = av_guess_format(NULL, "save.aac", NULL);
			_aac_fmt_ctx->url = av_strdup("save.aac");

			if (_aac_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
				_aac_fmt_ctx->oformat->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			_aac_stream = avformat_new_stream(_aac_fmt_ctx, NULL);
			if (!_aac_stream) {
				err = AE_FFMPEG_CREATE_STREAM_FAILED;
				break;
			}

			ret = avcodec_parameters_from_context(_aac_stream->codecpar, _encoder_ctx);
			if (ret < 0) {
				err = AE_FFMPEG_COPY_PARAMS_FAILED;
				break;
			}
#endif

			_inited = true;
		
		} while (0);

		if (err != AE_NO) {
			al_debug("%s,error:%d", err2str(err), ret);
			cleanup();
		}

		return err;
	}

	int encoder_aac::start()
	{
		int error = AE_NO;

		if (_running == true) {
			return error;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&encoder_aac::encode_loop, this));

		return error;
	}

	void encoder_aac::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();
	}

	int encoder_aac::put(const uint8_t * data, int data_len)
	{
		_ring_buffer->put(data, data_len);
		return 0;
	}

	void encoder_aac::encode_loop()
	{
		int len, ret = 0;

		AVPacket packet;

#ifdef SAVE_AAC
		avformat_write_header(_aac_fmt_ctx, NULL);
#endif

		while (_running)
		{
			len = _ring_buffer->get(_buff, _buff_size);
			if (!len)
				continue;

			ret = avcodec_send_frame(_encoder_ctx, _frame);

			if (ret == 0) {

				av_init_packet(&packet);
				ret = avcodec_receive_packet(_encoder_ctx, &packet);

				if (ret == 0) {
					if (_on_data) _on_data(packet.data, packet.size);

#ifdef SAVE_AAC
					av_write_frame(_aac_fmt_ctx, &packet);
#endif

					av_packet_unref(&packet);
				}
				else {
					//if (_on_error) _on_error(AE_FFMPEG_READ_PACKET_FAILED);

					al_fatal("read aac packet failed:%d", ret);
				}
			}
			else {
				if (_on_error) _on_error(AE_FFMPEG_ENCODE_FRAME_FAILED);

				al_fatal("encode pcm failed:%d", ret);
			}
		}

#ifdef SAVE_AAC
		av_write_trailer(_aac_fmt_ctx);
#endif
	}

	void encoder_aac::cleanup()
	{
		if (_encoder) {
			avcodec_close(_encoder_ctx);
		}

		_encoder = NULL;

		if (_encoder_ctx) {
			avcodec_free_context(&_encoder_ctx);
		}

		if (_frame)
			av_free(_frame);
		_frame = NULL;

		if (_buff)
			av_free(_buff);

		_buff = NULL;

		_encoder_ctx = NULL;

#ifdef SAVE_AAC
		if (_aac_fmt_ctx) {
			avio_closep(&_aac_fmt_ctx->pb);
			avformat_free_context(_aac_fmt_ctx);
		}
#endif

	}
}