#include "record_desktop_ffmpeg_dshow.h"

#include "error_define.h"

#include "utils\ray_log.h"

namespace am {

	record_desktop_ffmpeg_dshow::record_desktop_ffmpeg_dshow()
	{
		av_register_all();
		avdevice_register_all();

		_fmt_ctx = NULL;
		_input_fmt = NULL;
		_codec_ctx = NULL;
		_codec = NULL;

		_stream_index = -1;
		_data_type = RECORD_DESKTOP_DATA_TYPES::AT_DESKTOP_RGBA;
	}


	record_desktop_ffmpeg_dshow::~record_desktop_ffmpeg_dshow()
	{
		stop();
		clean_up();
	}

	int record_desktop_ffmpeg_dshow::init(const RECORD_DESKTOP_RECT & rect, const int fps)
	{
		int error = AE_NO;
		if (_inited == true) {
			return error;
		}

		_fps = fps;
		_rect = rect;

		char buff_video_size[50] = { 0 };
		sprintf_s(buff_video_size, 50, "%dx%d", rect.right - rect.left, rect.bottom - rect.top);

		AVDictionary *options = NULL;
		av_dict_set_int(&options, "framerate", fps, AV_DICT_MATCH_CASE);
		av_dict_set_int(&options, "offset_x", rect.left, AV_DICT_MATCH_CASE);
		av_dict_set_int(&options, "offset_y", rect.top, AV_DICT_MATCH_CASE);
		av_dict_set(&options, "video_size", buff_video_size, AV_DICT_MATCH_CASE);
		av_dict_set_int(&options, "draw_mouse", 1, AV_DICT_MATCH_CASE);

		int ret = 0;
		do {
			_fmt_ctx = avformat_alloc_context();
			_input_fmt = av_find_input_format("dshow");

			//the framerate must be same like encoder & muxer 's framerate,otherwise the video can not sync with audio
			ret = avformat_open_input(&_fmt_ctx, "video=screen-capture-recorder", _input_fmt, &options);
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
				if (_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
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

			_start_time = _fmt_ctx->streams[_stream_index]->start_time;
			_time_base = _fmt_ctx->streams[_stream_index]->time_base;
			_pixel_fmt = _fmt_ctx->streams[_stream_index]->codec->pix_fmt;

			_inited = true;
		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "record desktop ffmpeg dshow init failed: " << (err2str(error)) << " ,ret: " << ret;
			clean_up();
		}

		av_dict_free(&options);

		return error;
	}

	int record_desktop_ffmpeg_dshow::start()
	{
		if (_running == true) {
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&record_desktop_ffmpeg_dshow::record_func, this));

		return AE_NO;
	}

	int record_desktop_ffmpeg_dshow::pause()
	{
		return 0;
	}

	int record_desktop_ffmpeg_dshow::resume()
	{
		return 0;
	}

	int record_desktop_ffmpeg_dshow::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	void record_desktop_ffmpeg_dshow::clean_up()
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

	int record_desktop_ffmpeg_dshow::decode(AVFrame * frame, AVPacket * packet)
	{
		int ret = avcodec_send_packet(_codec_ctx, packet);
		if (ret < 0) {
			LOG(ERROR) << "avcodec_send_packet failed: " << ret;

			return AE_FFMPEG_DECODE_FRAME_FAILED;
		}

		while (ret >=0)
		{
			ret = avcodec_receive_frame(_codec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}

			if (ret < 0) {
				return AE_FFMPEG_READ_FRAME_FAILED;
			}

			if (ret == 0 && _on_data)
				_on_data(frame);

			av_frame_unref(frame);//need to do this? avcodec_receive_frame said will call unref before receive
		}

		return AE_NO;
	}

	void record_desktop_ffmpeg_dshow::record_func()
	{
		AVPacket *packet = av_packet_alloc();
		AVFrame *frame = av_frame_alloc();

		int ret = 0;

		int got_pic = 0;
		while (_running == true) {

			av_init_packet(packet);

			ret = av_read_frame(_fmt_ctx, packet);

			if (ret < 0) {
				if (_on_error) _on_error(AE_FFMPEG_READ_FRAME_FAILED);

				LOG(FATAL) << "read frame failed: " << ret;
				break;
			}

			if (packet->stream_index == _stream_index) {
				
				ret = decode(frame, packet);
				if (ret != AE_NO) {
					if (_on_error) _on_error(AE_FFMPEG_DECODE_FRAME_FAILED);
					LOG(FATAL) << "decode desktop frame failed";
					break;
				}
			}

			av_packet_unref(packet);
		}

		//flush packet in decoder
		decode(frame, NULL);

		av_packet_free(&packet);
		av_frame_free(&frame);
	}

}