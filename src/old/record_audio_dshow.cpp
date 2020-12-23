#include "record_audio_dshow.h"

#include "error_define.h"

#include "utils\ray_log.h"

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

	int record_audio_dshow::init(const std::string & device_name, const std::string &device_id, bool is_input)
	{
		int error = AE_NO;
		int ret = 0;

		if (_inited == true)
			return error;

		do {

			_device_name = device_name;
			_device_id = device_id;
			_is_input = is_input;

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
			_fmt = _codec_ctx->sample_fmt;

			_inited = true;
		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "record dshow init failed:" <<  (err2str(error)) << " ,ret: " << ret;
			cleanup();
		}

		return error;
	}

	int record_audio_dshow::start()
	{
		if (_running == true) {
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

	const AVRational record_audio_dshow::get_time_base()
	{
		if (_inited && _fmt_ctx && _stream_index != -1) {
			return _fmt_ctx->streams[_stream_index]->time_base;
		}
		else {
			return{ 1,AV_TIME_BASE };
		}
	}

	int64_t record_audio_dshow::get_start_time()
	{
		return _fmt_ctx->streams[_stream_index]->start_time;
	}

	int record_audio_dshow::decode(AVFrame * frame, AVPacket * packet)
	{
		int ret = avcodec_send_packet(_codec_ctx, packet);
		if (ret < 0) {
			LOG(ERROR) << "avcodec_send_packet failed:" << ret;

			return AE_FFMPEG_DECODE_FRAME_FAILED;
		}

		while (ret >= 0)
		{
			ret = avcodec_receive_frame(_codec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}

			if (ret < 0) {
				return AE_FFMPEG_READ_FRAME_FAILED;
			}

			if (ret == 0 && _on_data)
				_on_data(frame, _cb_extra_index);

			av_frame_unref(frame);//need to do this? avcodec_receive_frame said will call unref before receive
		}

		return AE_NO;
	}

	void record_audio_dshow::record_loop()
	{
		int ret = 0;

		AVPacket *packet = av_packet_alloc();

		AVFrame *frame = av_frame_alloc();

		while (_running == true) {
			av_init_packet(packet);

			ret = av_read_frame(_fmt_ctx, packet);

			if (ret < 0) {
				if (_on_error) _on_error(AE_FFMPEG_READ_FRAME_FAILED, _cb_extra_index);
				break;
			}

			if (packet->stream_index == _stream_index) {
				ret = decode(frame, packet);
				if (ret != AE_NO) {
					if (_on_error) _on_error(AE_FFMPEG_DECODE_FRAME_FAILED, _cb_extra_index);

					LOG(FATAL) << "decode pcm packet failed:" << ret;
					break;
				}
			}

			av_packet_unref(packet);
		}

		//flush packet left in decoder
		decode(frame, NULL);

		av_packet_free(&packet);
		av_frame_free(&frame);
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