#include "muxer_libmp4v2.h"
#include "muxer_define.h"

#include "record_audio.h"
#include "record_desktop.h"
#include "encoder_264.h"
#include "encoder_aac.h"
#include "resample_pcm.h"
#include "filter_amix.h"
#include "ring_buffer.h"

#include "log_helper.h"
#include "error_define.h"

extern "C" {
#include <libavutil\avassert.h>
#include <libavutil\channel_layout.h>
#include <libavutil\opt.h>
#include <libavutil\mathematics.h>
#include <libavutil\timestamp.h>
#include <libavutil\error.h>
}

namespace am {

	muxer_libmp4v2::muxer_libmp4v2()
	{
		av_register_all();

		_have_a = false;
		_have_v = false;

		_output_file = "";

		_v_stream = NULL;
		_a_stream = NULL;

		_fmt = NULL;
		_fmt_ctx = NULL;

		_inited = false;
		_running = false;
	}

	muxer_libmp4v2::~muxer_libmp4v2()
	{
		stop();
		cleanup();
	}

	int muxer_libmp4v2::init(
		const char * output_file,
		record_desktop * source_desktop,
		record_audio ** source_audios,
		const int source_audios_nb,
		const MUX_SETTING_T & setting
	)
	{
		int error = AE_NO;
		int ret = 0;

		do {
			error = alloc_oc(output_file, setting);
			if (error != AE_NO)
				break;

			if (_fmt->video_codec != AV_CODEC_ID_NONE) {
				error = add_video_stream(setting, source_desktop);
				if (error != AE_NO)
					break;
			}

			if (_fmt->audio_codec != AV_CODEC_ID_NONE) {
				error = add_audio_stream(setting, source_audios, source_audios_nb);
				if (error != AE_NO)
					break;
			}

			error = open_output(output_file, setting);
			if (error != AE_NO)
				break;

			av_dump_format(_fmt_ctx, 0, NULL, 1);

			_inited = true;

		} while (0);

		if (error != AE_NO) {
			cleanup();
			al_debug("muxer mp4 initialize failed:%s %d", err2str(error), ret);
		}

		return error;
	}

	int muxer_libmp4v2::start()
	{
		int error = AE_NO;

		if (_running == true) {
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		if (_v_stream && _v_stream->v_enc)
			_v_stream->v_enc->start();

		if (_v_stream && _v_stream->v_src)
			_v_stream->v_src->start();

		if (_a_stream) {
			if (_a_stream->a_enc)
				_a_stream->a_enc->start();

			for (int i = 0; i < _a_stream->a_nb; i++) {
				_a_stream->a_src[i]->start();
			}
		}


		_running = true;
		_thread = std::thread(std::bind(&muxer_libmp4v2::mux_loop, this));

		return error;
	}

	int muxer_libmp4v2::stop()
	{
		_running = false;

		/*if (_v_stream && _v_stream->v_src)
		_v_stream->v_src->stop();

		if (_v_stream && _v_stream->v_enc)
		_v_stream->v_enc->stop();*/

		if (_a_stream) {
			/*for (int i = 0; i < _a_stream->a_nb; i++) {
			_a_stream->a_src[i]->stop();
			}*/

			if (_a_stream->a_enc)
				_a_stream->a_enc->stop();
		}

		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	int muxer_libmp4v2::pause()
	{
		return 0;
	}

	int muxer_libmp4v2::resume()
	{
		return 0;
	}

	void muxer_libmp4v2::on_desktop_data(const uint8_t * data, int len)
	{
		if (_running && _v_stream && _v_stream->v_enc) {
			_v_stream->v_enc->put(data, len);
		}
	}

	void muxer_libmp4v2::on_desktop_error(int error)
	{
		al_fatal("on desktop capture error:%d", error);
	}

	void muxer_libmp4v2::on_audio_data(const uint8_t * data, int len, int index)
	{
		if (_running == false
			|| !_a_stream
			|| !_a_stream->a_samples
			|| !_a_stream->a_samples[index]
			|| !_a_stream->a_resamples
			|| !_a_stream->a_resamples[index]
			|| !_a_stream->a_rs
			|| !_a_stream->a_rs[index])
			return;

		AUDIO_SAMPLE *samples = _a_stream->a_samples[index];
		AUDIO_SAMPLE *resamples = _a_stream->a_resamples[index];
		resample_pcm *resampler = _a_stream->a_rs[index];

		//cache pcm
		int copied_len = min(samples->size - samples->sample_in, len);
		if (copied_len) {
			memcpy(samples->buff + samples->sample_in, data, copied_len);
			samples->sample_in += copied_len;
		}

		//got enough pcm to encoder,resample and mix
		if (samples->sample_in == samples->size) {
			int ret = resampler->convert(samples->buff, samples->size, resamples->buff, resamples->size);
			if (ret > 0) {
				_a_stream->a_enc->put(resamples->buff, resamples->size);
			}
			else {
				al_debug("resample audio %d failed,%d", index, ret);
			}

			samples->sample_in = 0;
		}

		//copy last pcms
		if (len - copied_len > 0) {
			memcpy(samples->buff + samples->sample_in, data + copied_len, len - copied_len);
			samples->sample_in += len - copied_len;
		}
	}

	void muxer_libmp4v2::on_audio_error(int error, int index)
	{
		al_fatal("on audio capture error:%d with stream index:%d", error, index);
	}

	void muxer_libmp4v2::on_enc_264_data(const uint8_t * data, int len, bool key_frame)
	{
		//al_debug("on video data:%d", len);
		if (_running && _v_stream && _v_stream->buffer) {
			//write_video(data, len, key_frame);
			_v_stream->buffer->put(data, len);
		}
	}

	void muxer_libmp4v2::on_enc_264_error(int error)
	{
		al_fatal("on desktop encode error:%d", error);
	}

	void muxer_libmp4v2::on_enc_aac_data(const uint8_t * data, int len)
	{
		//al_debug("on audio data:%d", len);
		if (_running && _a_stream && _a_stream->buffer) {
			//write_audio(data, len);
			_a_stream->buffer->put(data, len);
		}
	}

	void muxer_libmp4v2::on_enc_aac_error(int error)
	{
		al_fatal("on audio encode error:%d", error);
	}

	int muxer_libmp4v2::alloc_oc(const char * output_file, const MUX_SETTING_T & setting)
	{
		_output_file = std::string(output_file);

		int error = AE_NO;
		int ret = 0;

		do {
			ret = avformat_alloc_output_context2(&_fmt_ctx, NULL, NULL, output_file);
			if (ret < 0 || !_fmt_ctx) {
				error = AE_FFMPEG_ALLOC_CONTEXT_FAILED;
				break;
			}

			_fmt = _fmt_ctx->oformat;
		} while (0);

		return error;
	}

	int muxer_libmp4v2::add_video_stream(const MUX_SETTING_T & setting, record_desktop * source_desktop)
	{
		int error = AE_NO;
		int ret = 0;

		_v_stream = new MUX_STREAM();
		memset(_v_stream, 0, sizeof(MUX_STREAM));

		_v_stream->v_src = source_desktop;

		_v_stream->v_src->registe_cb(
			std::bind(&muxer_libmp4v2::on_desktop_data, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&muxer_libmp4v2::on_desktop_error, this, std::placeholders::_1)
		);

		int width = _v_stream->v_src->get_rect().right - _v_stream->v_src->get_rect().left;
		int height = _v_stream->v_src->get_rect().bottom - _v_stream->v_src->get_rect().top;

		do {
			_v_stream->v_enc = new encoder_264();
			error = _v_stream->v_enc->init(width, height, setting.v_frame_rate, setting.v_bit_rate, NULL);
			if (error != AE_NO)
				break;

			_v_stream->v_enc->registe_cb(
				std::bind(&muxer_libmp4v2::on_enc_264_data, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
				std::bind(&muxer_libmp4v2::on_enc_264_error, this, std::placeholders::_1)
			);

			AVCodec *codec = avcodec_find_encoder(_fmt->video_codec);
			if (!codec) {
				error = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}

			AVStream *st = avformat_new_stream(_fmt_ctx, codec);
			if (!st) {
				error = AE_FFMPEG_NEW_STREAM_FAILED;
				break;
			}

			st->codec->codec_id = AV_CODEC_ID_H264;
			st->codec->bit_rate_tolerance = setting.v_bit_rate;
			st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
			st->codec->time_base.den = setting.v_frame_rate;
			st->codec->time_base.num = 1;
			st->codec->pix_fmt = AV_PIX_FMT_YUV420P;

			st->codec->coded_width = width;
			st->codec->coded_height = height;
			st->codec->width = width;
			st->codec->height = height;
			//st->codec->max_b_frames = 2;
			//st->codec->extradata = 
			//st->codec->extradata_size = 
			st->time_base = st->codec->time_base;
			st->avg_frame_rate = av_inv_q(st->codec->time_base);

			if (_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {//without this,normal player can not play
				st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				st->codec->extradata_size = _v_stream->v_enc->get_extradata_size();// +AV_INPUT_BUFFER_PADDING_SIZE;
				st->codec->extradata = (uint8_t*)av_memdup(_v_stream->v_enc->get_extradata(), _v_stream->v_enc->get_extradata_size());
			}

			_v_stream->st = st;

			_v_stream->filter = av_bitstream_filter_init("h264_mp4toannexb");

			_v_stream->buffer = new ring_buffer(1024 * 1024 * 10);
		} while (0);

		return error;
	}

	static AVSampleFormat AFTypetrans2AVType(RECORD_AUDIO_FORMAT af_format) {
		switch (af_format) {
		case AF_AUDIO_S16:
			return AV_SAMPLE_FMT_S16;
		case AF_AUDIO_S32:
			return AV_SAMPLE_FMT_S32;
		case AF_AUDIO_FLT:
			return AV_SAMPLE_FMT_FLT;
		case AF_AUDIO_S16P:
			return AV_SAMPLE_FMT_S16P;
		case AF_AUDIO_S32P:
			return AV_SAMPLE_FMT_S32P;
		case AF_AUDIO_FLTP:
			return AV_SAMPLE_FMT_FLTP;
		}
	}

	int muxer_libmp4v2::add_audio_stream(const MUX_SETTING_T & setting, record_audio ** source_audios, const int source_audios_nb)
	{
		int error = AE_NO;
		int ret = 0;

		_a_stream = new MUX_STREAM();
		memset(_a_stream, 0, sizeof(MUX_STREAM));


		_a_stream->a_nb = source_audios_nb;
		_a_stream->a_rs = new resample_pcm*[_a_stream->a_nb];
		_a_stream->a_resamples = new AUDIO_SAMPLE*[_a_stream->a_nb];
		_a_stream->a_samples = new AUDIO_SAMPLE*[_a_stream->a_nb];
		_a_stream->a_src = source_audios;

		do {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				_a_stream->a_src[i]->registe_cb(
					std::bind(&muxer_libmp4v2::on_audio_data, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
					std::bind(&muxer_libmp4v2::on_audio_error, this, std::placeholders::_1, std::placeholders::_2),
					i
				);

				SAMPLE_SETTING src_setting = {
					1024,
					av_get_default_channel_layout(_a_stream->a_src[i]->get_channel_num()),
					_a_stream->a_src[i]->get_channel_num(),
					AFTypetrans2AVType(_a_stream->a_src[i]->get_fmt()),
					_a_stream->a_src[i]->get_sample_rate()
				};
				SAMPLE_SETTING dst_setting = {
					setting.a_nb_samples,
					av_get_default_channel_layout(setting.a_nb_channel),
					setting.a_nb_channel,
					setting.a_sample_fmt,
					setting.a_sample_rate
				};

				_a_stream->a_rs[i] = new resample_pcm();
				_a_stream->a_resamples[i] = new AUDIO_SAMPLE({ NULL,0,0 });
				_a_stream->a_rs[i]->init(&src_setting, &dst_setting, &_a_stream->a_resamples[i]->size);
				_a_stream->a_resamples[i]->buff = new uint8_t[_a_stream->a_resamples[i]->size];

				_a_stream->a_samples[i] = new AUDIO_SAMPLE({ NULL,0,0 });
				_a_stream->a_samples[i]->size = av_samples_get_buffer_size(NULL, src_setting.nb_channels, src_setting.nb_samples, src_setting.fmt, 0);
				_a_stream->a_samples[i]->buff = new uint8_t[_a_stream->a_samples[i]->size];
			}

			_a_stream->a_enc = new encoder_aac();
			error = _a_stream->a_enc->init(
				setting.a_nb_channel,
				setting.a_nb_samples,
				setting.a_sample_rate,
				setting.a_sample_fmt,
				setting.a_bit_rate
			);
			if (error != AE_NO)
				break;

			_a_stream->a_enc->registe_cb(
				std::bind(&muxer_libmp4v2::on_enc_aac_data, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&muxer_libmp4v2::on_enc_aac_error, this, std::placeholders::_1)
			);

			AVCodec *codec = avcodec_find_encoder(_fmt->audio_codec);
			if (!codec) {
				error = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}

			AVStream *st = avformat_new_stream(_fmt_ctx, codec);
			if (!st) {
				error = AE_FFMPEG_NEW_STREAM_FAILED;
				break;
			}

			st->time_base.num = 1;
			st->time_base.den = setting.a_sample_rate;

			st->codec->bit_rate = setting.a_bit_rate;
			st->codec->channels = setting.a_nb_channel;
			st->codec->sample_rate = setting.a_sample_rate;
			st->codec->sample_fmt = setting.a_sample_fmt;
			st->codec->time_base = st->time_base;
			//st->codec->extradata = 
			//st->codec->extradata_size = 
			st->codec->channel_layout = av_get_default_channel_layout(setting.a_nb_channel);

			if (_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {//without this,normal player can not play
				st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				st->codec->extradata_size = _a_stream->a_enc->get_extradata_size();// +AV_INPUT_BUFFER_PADDING_SIZE;
				st->codec->extradata = (uint8_t*)av_memdup(_a_stream->a_enc->get_extradata(), _a_stream->a_enc->get_extradata_size());
			}

			_a_stream->st = st;

			_a_stream->filter = av_bitstream_filter_init("aac_adtstoasc");

			_a_stream->buffer = new ring_buffer(1024 * 1024 * 10);

		} while (0);

		return error;
	}

	int muxer_libmp4v2::open_output(const char * output_file, const MUX_SETTING_T & setting)
	{
		int error = AE_NO;
		int ret = 0;

		do {
			ret = avio_open(&_fmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
			if (ret < 0) {
				error = AE_FFMPEG_OPEN_IO_FAILED;
				break;
			}

			ret = avformat_write_header(_fmt_ctx, NULL);
		} while (0);

		return error;
	}

	void muxer_libmp4v2::cleanup_video()
	{
		if (!_v_stream)
			return;

		if (_v_stream->frame)
			av_frame_free(&_v_stream->frame);

		if (_v_stream->tmp_frame)
			av_frame_free(&_v_stream->tmp_frame);

		if (_v_stream->buffer)
			delete _v_stream->buffer;

		if (_v_stream->v_enc)
			delete _v_stream->v_enc;

		delete _v_stream;

		_v_stream = nullptr;
	}

	void muxer_libmp4v2::cleanup_audio()
	{
		if (!_a_stream)
			return;

		if (_a_stream->frame)
			av_frame_free(&_a_stream->frame);

		if (_a_stream->tmp_frame)
			av_frame_free(&_a_stream->tmp_frame);

		if (_a_stream->buffer)
			delete _a_stream->buffer;

		if (_a_stream->a_enc)
			delete _a_stream->a_enc;

		if (_a_stream->a_filter)
			delete _a_stream->a_filter;

		if (_a_stream->a_nb) {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				/*if (_a_stream->a_src && _a_stream->a_src[i])
				delete _a_stream->a_src[i];*/

				if (_a_stream->a_rs && _a_stream->a_rs[i])
					delete _a_stream->a_rs[i];

				if (_a_stream->a_samples && _a_stream->a_samples[i]) {
					delete[] _a_stream->a_samples[i]->buff;
					delete _a_stream->a_samples[i];
				}

				if (_a_stream->a_resamples && _a_stream->a_resamples[i]) {
					delete[] _a_stream->a_resamples[i]->buff;
					delete _a_stream->a_resamples[i];
				}
			}

			if (_a_stream->a_rs)
				delete[] _a_stream->a_rs;

			if (_a_stream->a_samples)
				delete[] _a_stream->a_samples;

			if (_a_stream->a_resamples)
				delete[] _a_stream->a_resamples;
		}

		delete _a_stream;

		_a_stream = nullptr;
	}

	void muxer_libmp4v2::cleanup()
	{
		cleanup_video();
		cleanup_audio();

		if (_fmt && !(_fmt->flags & AVFMT_NOFILE))
			avio_closep(&_fmt_ctx->pb);

		if (_fmt_ctx) {
			avformat_free_context(_fmt_ctx);
		}

		_fmt_ctx = NULL;
		_fmt = NULL;

		_inited = false;
	}

	int muxer_libmp4v2::write_video(const uint8_t * data, int len, bool key_frame)
	{
		AVPacket packet;
		av_init_packet(&packet);

		AVRational v_time_base = { 1,90000 };//should be input AVStream time base

		packet.data = (uint8_t*)data;
		packet.size = len;
		packet.stream_index = _v_stream->st->index;

		int64_t calc_duration = (double)AV_TIME_BASE / (double)_v_stream->v_src->get_frame_rate();
		packet.pts = (double)(_v_stream->cur_frame_index *calc_duration) / (double)(av_q2d(v_time_base)*AV_TIME_BASE);
		packet.dts = packet.pts;
		packet.pos = -1;
		packet.duration = (double)calc_duration / (double)(av_q2d(v_time_base)*AV_TIME_BASE);

		_v_stream->cur_pts = packet.pts;
		_v_stream->cur_frame_index++;

		//convert pts/dts to out_put timebase

		packet.pts = av_rescale_q_rnd(
			packet.pts,
			v_time_base,
			_v_stream->st->time_base,
			(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
		);

		packet.dts = av_rescale_q_rnd(
			packet.dts,
			v_time_base,
			_v_stream->st->time_base,
			(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
		);

		packet.pos = -1;
		packet.duration = av_rescale_q(packet.duration, v_time_base, _v_stream->st->time_base);

		if (key_frame == true)
			packet.flags = AV_PKT_FLAG_KEY;

		return av_interleaved_write_frame(_fmt_ctx, &packet);
	}

	int muxer_libmp4v2::write_audio(const uint8_t * data, int len)
	{
		AVPacket packet;
		av_init_packet(&packet);

		AVRational a_time_base = { 1,48000 };

		packet.data = (uint8_t*)data;
		packet.size = len;
		packet.stream_index = _a_stream->st->index;

		packet.pts = 1024 * 2 * _a_stream->cur_frame_index;//nb_samples * nb_channels * current frame index
		packet.dts = packet.pts;
		packet.duration = 1024;

		_a_stream->cur_frame_index++;
		_a_stream->cur_pts = packet.pts;


		packet.pts = av_rescale_q_rnd(
			packet.pts,
			a_time_base,
			_a_stream->st->time_base,
			(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
		);

		packet.dts = av_rescale_q_rnd(
			packet.dts,
			a_time_base,
			_a_stream->st->time_base,
			(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
		);

		packet.pos = -1;
		packet.duration = av_rescale_q(packet.duration, a_time_base, _a_stream->st->time_base);

		return av_interleaved_write_frame(_fmt_ctx, &packet);
	}

	//static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
	//{
	//	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

	//	printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
	//		av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
	//		av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
	//		av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
	//		pkt->stream_index);
	//}

	void muxer_libmp4v2::mux_loop()
	{
		AVPacket packet = { 0 };

		int ret = 0;
		int buf_size = 1024 * 1024 * 2;
		uint8_t *buf = new uint8_t[buf_size];

		AVRational v_time_base = { 1,90000 };//should be input AVStream time base
		AVRational a_time_base = { 1,48000 };

		while (_running) {
			av_init_packet(&packet);

			if (av_compare_ts(
				_v_stream->cur_pts, v_time_base,
				_a_stream->cur_pts, a_time_base
			) <= 0) {//write video
				ret = _v_stream->buffer->get(buf, buf_size);
				if (ret) {
					packet.data = buf;
					packet.size = ret;
					packet.stream_index = _v_stream->st->index;

					int64_t calc_duration = (double)AV_TIME_BASE / (double)_v_stream->v_src->get_frame_rate();
					packet.pts = (double)(_v_stream->cur_frame_index *calc_duration) / (double)(av_q2d(v_time_base)*AV_TIME_BASE);
					packet.dts = packet.pts;
					packet.pos = -1;
					packet.duration = (double)calc_duration / (double)(av_q2d(v_time_base)*AV_TIME_BASE);

					_v_stream->cur_pts = packet.pts;
					_v_stream->cur_frame_index++;

					//convert pts/dts to out_put timebase

					packet.pts = av_rescale_q_rnd(
						packet.pts,
						v_time_base,
						_v_stream->st->time_base,
						(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
					);

					packet.dts = av_rescale_q_rnd(
						packet.dts,
						v_time_base,
						_v_stream->st->time_base,
						(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
					);

					packet.pos = -1;
					packet.duration = av_rescale_q(packet.duration, v_time_base, _v_stream->st->time_base);

					av_bitstream_filter_filter(_v_stream->filter, _v_stream->st->codec, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);

					av_interleaved_write_frame(_fmt_ctx, &packet);
				}
			}
			else {//write audio
				ret = _a_stream->buffer->get(buf, buf_size);
				if (ret) {
					packet.data = buf;
					packet.size = ret;
					packet.stream_index = _a_stream->st->index;


					//int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(a_time_base);

					packet.pts = 1024 * 2 * _a_stream->cur_frame_index;//nb_samples * nb_channels * current frame index
					packet.dts = packet.pts;
					packet.duration = 1024;

					_a_stream->cur_frame_index++;
					_a_stream->cur_pts = packet.pts;


					packet.pts = av_rescale_q_rnd(
						packet.pts,
						a_time_base,
						_a_stream->st->time_base,
						(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
					);

					packet.dts = av_rescale_q_rnd(
						packet.dts,
						a_time_base,
						_a_stream->st->time_base,
						(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
					);

					packet.pos = -1;
					packet.duration = av_rescale_q(packet.duration, a_time_base, _a_stream->st->time_base);

					//av_bitstream_filter_filter(_a_stream->filter, _a_stream->st->codec, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);

					av_interleaved_write_frame(_fmt_ctx, &packet);
				}
			}
		}

		delete[] buf;

		av_write_trailer(_fmt_ctx);
	}

}