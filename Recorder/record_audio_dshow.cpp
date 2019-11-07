#include "stdafx.h"
#include "record_audio_dshow.h"

#include "log_helper.h"
#include "error_define.h"

namespace am {

	record_audio_dshow::record_audio_dshow()
	{
		av_register_all();
		avdevice_register_all();

		_fmt_ctx = NULL;
		_input_fmt = NULL;
		_codec_ctx = NULL;
		_codec = NULL;

		_stream_index = -1;
	}


	record_audio_dshow::~record_audio_dshow()
	{
		stop();
		cleanup();
	}

	static RECORD_AUDIO_FORMAT AVTypeTrans2AFType(AVSampleFormat av_format)
	{
		switch (av_format) {
		case AV_SAMPLE_FMT_S16:
			return AF_AUDIO_S16;
		case AV_SAMPLE_FMT_S32:
			return AF_AUDIO_S32;
		case AV_SAMPLE_FMT_FLT:
			return AF_AUDIO_FLT;
		case AV_SAMPLE_FMT_S16P:
			return AF_AUDIO_S16P;
		case AV_SAMPLE_FMT_S32P:
			return AF_AUDIO_S32P;
		case AV_SAMPLE_FMT_FLTP:
			return AF_AUDIO_FLTP;
		}
	}

	static std::string  unicode_utf8(const std::wstring& wstr)
	{
		int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
		std::string ret_str = pAssii;
		free(pAssii);
		return ret_str;
	}

	static std::wstring ascii_unicode(const std::string & str)
	{
		int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
		wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
		std::wstring ret_str = pUnicode;
		free(pUnicode);
		return ret_str;
	}

	int record_audio_dshow::init(const std::string & device_name)
	{
		int error = AE_NO;
		int ret = 0;

		if (_inited == true)
			return error;

		do {

			_device_name = unicode_utf8(ascii_unicode(device_name));

			_input_fmt = av_find_input_format("dshow");
			if (!_input_fmt) {
				error = AE_FFMPEG_FIND_INPUT_FMT_FAILED;
				break;
			}

			_fmt_ctx = avformat_alloc_context();
			ret = avformat_open_input(&_fmt_ctx, _device_name.c_str(), _input_fmt, NULL);
			if (ret != 0) {
				error = AE_FFMPEG_OPEN_INPUT_FAILED;
				break;
			}


			ret = avformat_find_stream_info(_fmt_ctx, NULL);
			if (ret < 0) {
				error = AE_FFMPEG_FIND_STREAM_FAILED;
				break;
			}

			int stream_index = -1;
			for (int i = 0; i < _fmt_ctx->nb_streams; i++) {
				if (_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
					stream_index = i;
					break;
				}
			}

			if (stream_index == -1) {
				error = AE_FFMPEG_FIND_STREAM_FAILED;
				break;
			}

			_stream_index = stream_index;
			_codec_ctx = _fmt_ctx->streams[stream_index]->codec;
			_codec = avcodec_find_decoder(_codec_ctx->codec_id);
			if (_codec == NULL) {
				error = AE_FFMPEG_FIND_DECODER_FAILED;
				break;
			}

			ret = avcodec_open2(_codec_ctx, _codec, NULL);
			if (ret != 0) {
				error = AE_FFMPEG_OPEN_CODEC_FAILED;
				break;
			}

			_inited = true;

			_sample_rate = _codec_ctx->sample_rate;
			_bit_rate = _codec_ctx->bit_rate;
			_bit_per_sample = _codec_ctx->bits_per_coded_sample;
			_channel_num = _codec_ctx->channels;
			_fmt = AVTypeTrans2AFType(_codec_ctx->sample_fmt);

			_inited = true;
		} while (0);

		if (error != AE_NO) {
			al_debug("%s,error:%d", err2str(error), ret);
			cleanup();
		}

		return error;
	}

	int record_audio_dshow::start()
	{
		if (_running == true) {
			al_warn("record audio dshow is already running");
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}


		_running = true;
		_thread = std::thread(std::bind(&record_audio_dshow::record_loop, this));

		return AE_NO;
	}

	int record_audio_dshow::pause()
	{
		return 0;
	}

	int record_audio_dshow::resume()
	{
		return 0;
	}

	int record_audio_dshow::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	const AVRational & record_audio_dshow::get_time_base()
	{
		if (_inited && _fmt_ctx && _stream_index != -1) {
			return _fmt_ctx->streams[_stream_index]->time_base;
		}
		else {
			return{ 1,90000 };
		}
	}

	void record_audio_dshow::record_loop()
	{
		int ret = 0;

		AVPacket *packet = av_packet_alloc();
		
		av_init_packet(packet);

		AVFrame *frame = av_frame_alloc();

		int sample_len = 0;

		while (_running == true) {
			ret = av_read_frame(_fmt_ctx, packet);

			if (ret < 0) {
				if (_on_error) _on_error(AE_FFMPEG_READ_FRAME_FAILED, _cb_extra_index);

				al_fatal("read frame failed:%d %ld", );
				break;
			}

			if (packet->stream_index == _stream_index) {
				ret = avcodec_send_packet(_codec_ctx, packet);
				while (ret >= 0) {
					ret = avcodec_receive_frame(_codec_ctx, frame);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						break;
					}

					if (ret < 0) {
						if (_on_error) _on_error(AE_FFMPEG_READ_PACKET_FAILED, _cb_extra_index);

						al_fatal("read pcm frame failed:%d", ret);
						break;
					}

					if (_on_data) {
						sample_len = av_samples_get_buffer_size(NULL, _codec_ctx->channels, frame->nb_samples, _codec_ctx->sample_fmt, 1);
						_on_data(frame->data[0], sample_len, _cb_extra_index);
					}
				}
			}

			av_free_packet(packet);
		}
	}

	void record_audio_dshow::cleanup()
	{
		if (_codec_ctx)
			avcodec_close(_codec_ctx);

		if (_fmt_ctx)
			avformat_close_input(&_fmt_ctx);

		_fmt_ctx = NULL;
		_input_fmt = NULL;
		_codec_ctx = NULL;
		_codec = NULL;

		_stream_index = -1;
		_inited = false;
	}

}