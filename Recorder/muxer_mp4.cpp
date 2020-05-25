#include "muxer_mp4.h"
#include "muxer_define.h"

#include "record_desktop.h"
#include "encoder_264.h"
#include "sws_helper.h"

#include "record_audio.h"
#include "encoder_aac.h"
#include "filter_amix.h"
#include "filter_aresample.h"

#include "ring_buffer.h"

#include "log_helper.h"
#include "error_define.h"


namespace am {

	muxer_mp4::muxer_mp4()
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
		_paused = false;

		_base_time = -1;
	}

	muxer_mp4::~muxer_mp4()
	{
		stop();
		cleanup();
	}

	int muxer_mp4::init(
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

			if (_fmt->audio_codec != AV_CODEC_ID_NONE && source_audios_nb) {
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

	int muxer_mp4::start()
	{
		std::lock_guard<std::mutex> lock(_mutex);

		int error = AE_NO;

		if (_running == true) {
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_base_time = -1;

		if (_v_stream && _v_stream->v_enc)
			_v_stream->v_enc->start();

		if (_a_stream && _a_stream->a_enc)
			_a_stream->a_enc->start();

		if (_a_stream && _a_stream->a_nb >= 2 && _a_stream->a_filter_amix)
			_a_stream->a_filter_amix->start();

		if (_a_stream && _a_stream->a_nb < 2 && _a_stream->a_filter_aresample) {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				_a_stream->a_filter_aresample[i]->start();
			}
		}

		if (_a_stream && _a_stream->a_src) {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				if(_a_stream->a_src[i])
					_a_stream->a_src[i]->start();
			}
		}

		if (_v_stream && _v_stream->v_src)
			_v_stream->v_src->start();

		_running = true;

		return error;
	}

	int muxer_mp4::stop()
	{
		std::lock_guard<std::mutex> lock(_mutex);

		if (_running == false)
			return AE_NO;

		_running = false;

		al_debug("try to stop muxer....");

		al_debug("stop audio recorder...");
		if (_a_stream && _a_stream->a_src) {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				_a_stream->a_src[i]->stop();
			}
		}

		al_debug("stop video recorder...");
		if (_v_stream && _v_stream->v_src)
			_v_stream->v_src->stop();

		al_debug("stop audio amix filter...");
		if (_a_stream && _a_stream->a_filter_amix)
			_a_stream->a_filter_amix->stop();

		al_debug("stop audio aresampler filter...");
		if (_a_stream && _a_stream->a_filter_aresample) {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				_a_stream->a_filter_aresample[i]->stop();
			}
		}


		al_debug("stop video encoder...");
		if (_v_stream && _v_stream->v_enc)
			_v_stream->v_enc->stop();

		al_debug("stop audio encoder...");
		if (_a_stream) {
			if (_a_stream->a_enc)
				_a_stream->a_enc->stop();
		}

		al_debug("write mp4 trailer...");
		if (_fmt_ctx)
			av_write_trailer(_fmt_ctx);//must write trailer ,otherwise mp4 can not play

		al_debug("muxer stopped...");

		return AE_NO;
	}

	int muxer_mp4::pause()
	{
		_paused = true;

		return 0;
	}

	int muxer_mp4::resume()
	{
		_paused = false;
		return 0;
	}

	void muxer_mp4::on_desktop_data(AVFrame *frame)
	{
		if (_running == false || !_v_stream || !_v_stream->v_enc || !_v_stream->v_sws) {
			return;
		}

		int len = 0, ret = AE_NO;
		uint8_t *yuv_data = NULL;
		
		ret = _v_stream->v_sws->convert(frame, &yuv_data, &len);
		
		if (ret == AE_NO && yuv_data && len) {
			_v_stream->v_enc->put(yuv_data, len, frame);

			if (_on_yuv_data) _on_yuv_data(yuv_data, len, frame->width, frame->height, 0);
		}
	}

	void muxer_mp4::on_desktop_error(int error)
	{
		al_fatal("on desktop capture error:%d", error);
	}

	int getPcmDB(const unsigned char *pcmdata, size_t size) {

		int db = 0;
		float value = 0;
		double sum = 0;
		double average = 0;
		int bit_per_sample = 32;
		int byte_per_sample = bit_per_sample / 8;
		int channel_num = 2;

		for (int i = 0; i < size; i += channel_num * byte_per_sample)
		{
			memcpy(&value, pcmdata + i, byte_per_sample);
			sum += abs(value);
		}
		average = sum / (double)(size / byte_per_sample /channel_num);
		if (average > 0)
		{
			db = (int)(20 * log10f(average));
		}

		al_debug("%d   %f     %f", db, average,sum);
		return db;
	}

	void muxer_mp4::on_audio_data(AVFrame *frame, int index)
	{
		if (_running == false)
			return;

		if (_a_stream->a_filter_amix != nullptr)
			_a_stream->a_filter_amix->add_frame(frame, index);
		else if(_a_stream->a_filter_aresample != nullptr && _a_stream->a_filter_aresample[index] != nullptr) {
			_a_stream->a_filter_aresample[index]->add_frame(frame);
		}

		return;
	}

	void muxer_mp4::on_audio_error(int error, int index)
	{
		al_fatal("on audio capture error:%d with stream index:%d", error, index);
	}

	void muxer_mp4::on_filter_amix_data(AVFrame * frame, int)
	{
		if (_running == false || !_a_stream->a_enc)
			return;


		AUDIO_SAMPLE *resamples = _a_stream->a_resamples[0];

		int copied_len = 0;
		int sample_len = av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
		sample_len = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);

		int remain_len = sample_len;

		//for data is planar,should copy data[0] data[1] to correct buff pos
		if (av_sample_fmt_is_planar((AVSampleFormat)frame->format) == 0) {
			while (remain_len > 0) {
				//cache pcm
				copied_len = min(resamples->size - resamples->sample_in, remain_len);
				if (copied_len) {
					memcpy(resamples->buff + resamples->sample_in, frame->data[0] + sample_len - remain_len, copied_len);
					resamples->sample_in += copied_len;
					remain_len = remain_len - copied_len;
				}

				//got enough pcm to encoder,resample and mix
				if (resamples->sample_in == resamples->size) {
					_a_stream->a_enc->put(resamples->buff, resamples->size, frame);

					resamples->sample_in = 0;
				}
			}
		}
		else {//resample size is channels*frame->linesize[0],for 2 channels
			while (remain_len > 0) {
				copied_len = min(resamples->size - resamples->sample_in, remain_len);
				if (copied_len) {
					memcpy(resamples->buff + resamples->sample_in / 2, frame->data[0] + (sample_len - remain_len) / 2, copied_len / 2);
					memcpy(resamples->buff + resamples->size / 2 + resamples->sample_in / 2, frame->data[1] + (sample_len - remain_len) / 2, copied_len / 2);
					resamples->sample_in += copied_len;
					remain_len = remain_len - copied_len;
				}

				if (resamples->sample_in == resamples->size) {
					_a_stream->a_enc->put(resamples->buff, resamples->size, frame);

					resamples->sample_in = 0;
				}
			}
		}
	}

	void muxer_mp4::on_filter_amix_error(int error, int)
	{
		al_fatal("on filter amix audio error:%d", error);
	}

	void muxer_mp4::on_filter_aresample_data(AVFrame * frame, int index)
	{
		if (_running == false || !_a_stream->a_enc)
			return;


		AUDIO_SAMPLE *resamples = _a_stream->a_resamples[0];

		int copied_len = 0;
		int sample_len = av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
		sample_len = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);

		int remain_len = sample_len;

		//for data is planar,should copy data[0] data[1] to correct buff pos
		if (av_sample_fmt_is_planar((AVSampleFormat)frame->format) == 0) {
			while (remain_len > 0) {
				//cache pcm
				copied_len = min(resamples->size - resamples->sample_in, remain_len);
				if (copied_len) {
					memcpy(resamples->buff + resamples->sample_in, frame->data[0] + sample_len - remain_len, copied_len);
					resamples->sample_in += copied_len;
					remain_len = remain_len - copied_len;
				}

				//got enough pcm to encoder,resample and mix
				if (resamples->sample_in == resamples->size) {
					_a_stream->a_enc->put(resamples->buff, resamples->size, frame);

					resamples->sample_in = 0;
				}
			}
		}
		else {//resample size is channels*frame->linesize[0],for 2 channels
			while (remain_len > 0) {
				copied_len = min(resamples->size - resamples->sample_in, remain_len);
				if (copied_len) {
					memcpy(resamples->buff + resamples->sample_in / 2, frame->data[0] + (sample_len - remain_len) / 2, copied_len / 2);
					memcpy(resamples->buff + resamples->size / 2 + resamples->sample_in / 2, frame->data[1] + (sample_len - remain_len) / 2, copied_len / 2);
					resamples->sample_in += copied_len;
					remain_len = remain_len - copied_len;
				}

				if (resamples->sample_in == resamples->size) {
					_a_stream->a_enc->put(resamples->buff, resamples->size, frame);

					resamples->sample_in = 0;
				}
			}
		}
	}

	void muxer_mp4::on_filter_aresample_error(int error, int index)
	{
		al_fatal("on filter aresample[%d] audio error:%d", index, error);
	}

	void muxer_mp4::on_enc_264_data(AVPacket *packet)
	{
		if (_running && _v_stream) {
			write_video(packet);
		}
	}

	void muxer_mp4::on_enc_264_error(int error)
	{
		al_fatal("on desktop encode error:%d", error);
	}

	void muxer_mp4::on_enc_aac_data(AVPacket *packet)
	{
		if (_running && _a_stream) {
			write_audio(packet);
		}
	}

	void muxer_mp4::on_enc_aac_error(int error)
	{
		al_fatal("on audio encode error:%d", error);
	}

	int muxer_mp4::alloc_oc(const char * output_file, const MUX_SETTING_T & setting)
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

	int muxer_mp4::add_video_stream(const MUX_SETTING_T & setting, record_desktop * source_desktop)
	{
		int error = AE_NO;
		int ret = 0;

		_v_stream = new MUX_STREAM();
		memset(_v_stream, 0, sizeof(MUX_STREAM));

		_v_stream->v_src = source_desktop;

		_v_stream->pre_pts = -1;
		
		_v_stream->v_src->registe_cb(
			std::bind(&muxer_mp4::on_desktop_data, this, std::placeholders::_1),
			std::bind(&muxer_mp4::on_desktop_error, this, std::placeholders::_1)
		);

		RECORD_DESKTOP_RECT v_rect = _v_stream->v_src->get_rect();

		do {
			_v_stream->v_enc = new encoder_264();
			error = _v_stream->v_enc->init(setting.v_width, setting.v_height, setting.v_frame_rate,setting.v_bit_rate, setting.v_qb);
			if (error != AE_NO)
				break;

			_v_stream->v_enc->registe_cb(
				std::bind(&muxer_mp4::on_enc_264_data, this, std::placeholders::_1),
				std::bind(&muxer_mp4::on_enc_264_error, this, std::placeholders::_1)
			);

			_v_stream->v_sws = new sws_helper();
			error = _v_stream->v_sws->init(
				_v_stream->v_src->get_pixel_fmt(),
				v_rect.right - v_rect.left,
				v_rect.bottom - v_rect.top,
				AV_PIX_FMT_YUV420P,
				setting.v_width,
				setting.v_height
			);
			if (error != AE_NO)
				break;

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

			st->codec->coded_width = setting.v_width;
			st->codec->coded_height = setting.v_height;
			st->codec->width = setting.v_width;
			st->codec->height = setting.v_height;
			st->codec->max_b_frames = 0;//NO B Frame
			st->time_base = { 1,90000 };//fixed?
			st->avg_frame_rate = av_inv_q(st->codec->time_base);

			if (_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {//without this,normal player can not play,extradata will write with avformat_write_header
				st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				st->codec->extradata_size = _v_stream->v_enc->get_extradata_size();// +AV_INPUT_BUFFER_PADDING_SIZE;
				st->codec->extradata = (uint8_t*)av_memdup(_v_stream->v_enc->get_extradata(), _v_stream->v_enc->get_extradata_size());
			}

			_v_stream->st = st;

			_v_stream->setting = setting;
			_v_stream->filter = av_bitstream_filter_init("h264_mp4toannexb");
		} while (0);

		return error;
	}

	int muxer_mp4::add_audio_stream(const MUX_SETTING_T & setting, record_audio ** source_audios, const int source_audios_nb)
	{
		int error = AE_NO;
		int ret = 0;

		_a_stream = new MUX_STREAM();
		memset(_a_stream, 0, sizeof(MUX_STREAM));

		_a_stream->a_nb = source_audios_nb;
		_a_stream->a_filter_aresample = new filter_aresample*[_a_stream->a_nb];
		_a_stream->a_resamples = new AUDIO_SAMPLE*[_a_stream->a_nb];
		_a_stream->a_samples = new AUDIO_SAMPLE*[_a_stream->a_nb];
		_a_stream->a_src = new record_audio*[_a_stream->a_nb];
		_a_stream->pre_pts = -1;


		do {
			_a_stream->a_enc = new encoder_aac();
			error = _a_stream->a_enc->init(
				setting.a_nb_channel,
				setting.a_sample_rate,
				setting.a_sample_fmt,
				setting.a_bit_rate
			);
			if (error != AE_NO)
				break;

			_a_stream->a_enc->registe_cb(
				std::bind(&muxer_mp4::on_enc_aac_data, this, std::placeholders::_1),
				std::bind(&muxer_mp4::on_enc_aac_error, this, std::placeholders::_1)
			);

			for (int i = 0; i < _a_stream->a_nb; i++) {

				_a_stream->a_src[i] = source_audios[i];
				_a_stream->a_src[i]->registe_cb(
					std::bind(&muxer_mp4::on_audio_data, this, std::placeholders::_1, std::placeholders::_2),
					std::bind(&muxer_mp4::on_audio_error, this, std::placeholders::_1, std::placeholders::_2),
					i
				);

				_a_stream->a_filter_aresample[i] = new filter_aresample();
				_a_stream->a_resamples[i] = new AUDIO_SAMPLE({ NULL,0,0 });
				_a_stream->a_filter_aresample[i]->init({
					NULL,NULL,
					_a_stream->a_src[i]->get_time_base(),
					_a_stream->a_src[i]->get_sample_rate(),
					_a_stream->a_src[i]->get_fmt(),
					_a_stream->a_src[i]->get_channel_num(),
					av_get_default_channel_layout(_a_stream->a_src[i]->get_channel_num())
				}, {
					NULL,NULL,
					{ 1,AV_TIME_BASE },
					setting.a_sample_rate,
					setting.a_sample_fmt,
					setting.a_nb_channel,
					av_get_default_channel_layout(setting.a_nb_channel)
				}, i);

				_a_stream->a_filter_aresample[i]->registe_cb(
					std::bind(&muxer_mp4::on_filter_aresample_data, this, std::placeholders::_1, std::placeholders::_2),
					std::bind(&muxer_mp4::on_filter_aresample_error, this, std::placeholders::_1, std::placeholders::_2));

				_a_stream->a_resamples[i]->size = av_samples_get_buffer_size(
					NULL, setting.a_nb_channel, _a_stream->a_enc->get_nb_samples(), setting.a_sample_fmt, 1);
				_a_stream->a_resamples[i]->buff = new uint8_t[_a_stream->a_resamples[i]->size];
				_a_stream->a_samples[i] = new AUDIO_SAMPLE({ NULL,0,0 });
				_a_stream->a_samples[i]->size = av_samples_get_buffer_size(
					NULL, _a_stream->a_src[i]->get_channel_num(), _a_stream->a_enc->get_nb_samples(), _a_stream->a_src[i]->get_fmt(), 1);
				_a_stream->a_samples[i]->buff = new uint8_t[_a_stream->a_samples[i]->size];
			}

			if (_a_stream->a_nb >= 2) {
				_a_stream->a_filter_amix = new am::filter_amix();
				error = _a_stream->a_filter_amix->init(
				{
					NULL,NULL,
					_a_stream->a_src[0]->get_time_base(),
					_a_stream->a_src[0]->get_sample_rate(),
					_a_stream->a_src[0]->get_fmt(),
					_a_stream->a_src[0]->get_channel_num(),
					av_get_default_channel_layout(_a_stream->a_src[0]->get_channel_num())
				},
				{
					NULL,NULL,
					_a_stream->a_src[1]->get_time_base(),
					_a_stream->a_src[1]->get_sample_rate(),
					_a_stream->a_src[1]->get_fmt(),
					_a_stream->a_src[1]->get_channel_num(),
					av_get_default_channel_layout(_a_stream->a_src[1]->get_channel_num())
				},
				{
					NULL,NULL,
					{ 1,AV_TIME_BASE },
					setting.a_sample_rate,
					setting.a_sample_fmt,
					setting.a_nb_channel,
					av_get_default_channel_layout(setting.a_nb_channel)
				}
				);

				if (error != AE_NO) {
					break;
				}

				_a_stream->a_filter_amix->registe_cb(
					std::bind(&muxer_mp4::on_filter_amix_data, this, std::placeholders::_1, std::placeholders::_2),
					std::bind(&muxer_mp4::on_filter_amix_error, this, std::placeholders::_1, std::placeholders::_2)
				);
			}

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

			st->time_base = { 1,setting.a_sample_rate };

			st->codec->bit_rate = setting.a_bit_rate;
			st->codec->channels = setting.a_nb_channel;
			st->codec->sample_rate = setting.a_sample_rate;
			st->codec->sample_fmt = setting.a_sample_fmt;
			st->codec->time_base = { 1,setting.a_sample_rate };
			st->codec->channel_layout = av_get_default_channel_layout(setting.a_nb_channel);

			if (_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {//without this,normal player can not play
				st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				st->codec->extradata_size = _a_stream->a_enc->get_extradata_size();// +AV_INPUT_BUFFER_PADDING_SIZE;
				st->codec->extradata = (uint8_t*)av_memdup(_a_stream->a_enc->get_extradata(), _a_stream->a_enc->get_extradata_size());
			}

			_a_stream->st = st;		

			_a_stream->setting = setting;
			_a_stream->filter = av_bitstream_filter_init("aac_adtstoasc");

		} while (0);

		return error;
	}

	int muxer_mp4::open_output(const char * output_file, const MUX_SETTING_T & setting)
	{
		int error = AE_NO;
		int ret = 0;

		do {
			if (!(_fmt->flags & AVFMT_NOFILE)) {
				ret = avio_open(&_fmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
				if (ret < 0) {
					error = AE_FFMPEG_OPEN_IO_FAILED;
					break;
				}
			}

			AVDictionary* opt = NULL;
			av_dict_set_int(&opt, "video_track_timescale", _v_stream->setting.v_frame_rate, 0);

			//ret = avformat_write_header(_fmt_ctx, &opt);//no need to set this
			ret = avformat_write_header(_fmt_ctx, NULL);

			av_dict_free(&opt);

			if (ret < 0) {
				error = AE_FFMPEG_WRITE_HEADER_FAILED;
				break;
			}
		} while (0);

		return error;
	}

	void muxer_mp4::cleanup_video()
	{
		if (!_v_stream)
			return;

		if (_v_stream->v_enc)
			delete _v_stream->v_enc;

		if (_v_stream->v_sws)
			delete _v_stream->v_sws;

		delete _v_stream;

		_v_stream = nullptr;
	}

	void muxer_mp4::cleanup_audio()
	{
		if (!_a_stream)
			return;

		if (_a_stream->a_enc)
			delete _a_stream->a_enc;

		if (_a_stream->a_filter_amix)
			delete _a_stream->a_filter_amix;

		if (_a_stream->a_nb) {
			for (int i = 0; i < _a_stream->a_nb; i++) {
				if (_a_stream->a_filter_aresample && _a_stream->a_filter_aresample[i])
					delete _a_stream->a_filter_aresample[i];

				if (_a_stream->a_samples && _a_stream->a_samples[i]) {
					delete[] _a_stream->a_samples[i]->buff;
					delete _a_stream->a_samples[i];
				}

				if (_a_stream->a_resamples && _a_stream->a_resamples[i]) {
					delete[] _a_stream->a_resamples[i]->buff;
					delete _a_stream->a_resamples[i];
				}
			}

			if (_a_stream->a_filter_aresample)
				delete[] _a_stream->a_filter_aresample;

			if (_a_stream->a_samples)
				delete[] _a_stream->a_samples;

			if (_a_stream->a_resamples)
				delete[] _a_stream->a_resamples;
		}

		delete _a_stream;

		_a_stream = nullptr;
	}

	void muxer_mp4::cleanup()
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

	uint64_t muxer_mp4::get_current_time()
	{
		std::lock_guard<std::mutex> lock(_time_mutex);

		return av_gettime_relative();
	}

	int muxer_mp4::write_video(AVPacket *packet)
	{
		//must lock here,coz av_interleaved_write_frame will push packet into a queue,and is not thread safe
		std::lock_guard<std::mutex> lock(_mutex);

		if (_paused) return AE_NO;

		//if (_a_stream->pre_pts == (uint64_t)-1)
		//	return 0;

		packet->stream_index = _v_stream->st->index;

		if (_v_stream->pre_pts == (uint64_t)-1) {
			_v_stream->pre_pts = packet->pts;
		}

		packet->pts = packet->pts - _v_stream->pre_pts;
		//packet->pts = av_rescale_q_rnd(packet->pts, {1,AV_TIME_BASE}, _v_stream->st->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet->pts = av_rescale_q_rnd(packet->pts, _v_stream->v_src->get_time_base(), _v_stream->st->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));


		packet->dts = packet->pts;//make sure that dts is equal to pts


		//al_debug("V:%lld", packet->pts);

#if 0
		static FILE *fp = NULL;
		if (fp == NULL) {
			fp = fopen("..\\..\\save.264", "wb+");
			//write sps pps
			fwrite(_v_stream->v_enc->get_extradata(), 1, _v_stream->v_enc->get_extradata_size(), fp);
		}

		fwrite(packet->data, 1, packet->size, fp);

		fflush(fp);
#endif
		
		av_assert0(packet->data != NULL);

		int ret = av_interleaved_write_frame(_fmt_ctx, packet);//no need to unref packet,this will be auto unref

	}

	int muxer_mp4::write_audio(AVPacket *packet)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_paused) return AE_NO;

		
		packet->stream_index = _a_stream->st->index;

		if (_a_stream->pre_pts == (uint64_t)-1) {
			_a_stream->pre_pts = packet->pts;
		}

		packet->pts = packet->pts - _a_stream->pre_pts;

		if (_a_stream->a_filter_amix)
			packet->pts = av_rescale_q(packet->pts, _a_stream->a_filter_amix->get_time_base(), { 1,AV_TIME_BASE });
		else
			packet->pts = av_rescale_q(packet->pts, _a_stream->a_enc->get_time_base(), { 1,AV_TIME_BASE });

		packet->pts = av_rescale_q_rnd(packet->pts, { 1,AV_TIME_BASE }, _a_stream->st->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

		packet->dts = packet->pts;//make sure that dts is equal to pts

		//al_debug("A:%lld %lld", packet->pts, packet->dts);

		av_assert0(packet->data != NULL);

		int ret = av_interleaved_write_frame(_fmt_ctx, packet);//no need to unref packet,this will be auto unref

		return ret;
	}
}