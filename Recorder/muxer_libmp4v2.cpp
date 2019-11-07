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
#include <libavcodec\adts_parser.h>
#include <libavutil\time.h>
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

		_mp4v2_file = NULL;
		_mp4v2_v_track = -1;
		_mp4v2_a_track = -1;

		_base_time = -1;
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
			error = open_output(output_file, setting);
			if (error != AE_NO)
				break;

			error = add_video_stream(setting, source_desktop);
			if (error != AE_NO)
				break;

			error = add_audio_stream(setting, source_audios, source_audios_nb);
			if (error != AE_NO)
				break;

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

		_base_time = -1;

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

		return error;
	}

	int muxer_libmp4v2::stop()
	{
		_running = false;

		if (_a_stream) {
			if (_a_stream->a_enc)
				_a_stream->a_enc->stop();
		}

		if (_v_stream) {
			if (_v_stream->v_enc)
				_v_stream->v_enc->stop();
		}

		if(_mp4v2_file)
			MP4Close(_mp4v2_file);

		_mp4v2_file = NULL;

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
		if (_running ) {
			write_video(data, len, key_frame);
		}
	}

	void muxer_libmp4v2::on_enc_264_error(int error)
	{
		al_fatal("on desktop encode error:%d", error);
	}

	void muxer_libmp4v2::on_enc_aac_data(const uint8_t * data, int len)
	{
		//al_debug("on audio data:%d", len);
		if (_running ) {
			write_audio(data, len);
		}
	}

	void muxer_libmp4v2::on_enc_aac_error(int error)
	{
		al_fatal("on audio encode error:%d", error);
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

			_v_stream->setting = setting;

			const uint8_t *extradata = _v_stream->v_enc->get_extradata();
			int extradata_size = _v_stream->v_enc->get_extradata_size();

			int sps, pps;
			for (sps = 0; sps < extradata_size; sps++) {
				if (extradata[sps] == 0x00 && extradata[sps + 1] == 0x00 && extradata[sps + 2] == 0x00 && extradata[sps + 3] == 0x01) {
					sps += 4;
					break;
				}
			}

			for (pps = sps; pps < extradata_size; pps++) {
				if (extradata[pps] == 0x00 && extradata[pps + 1] == 0x00 && extradata[pps + 2] == 0x00 && extradata[pps + 3] == 0x01) {
					pps += 4;
					break;
				}
			}

			_mp4v2_v_track = MP4AddH264VideoTrack(
				_mp4v2_file,
				90000,
				90000/setting.v_frame_rate,
				width,
				height,
				extradata[sps + 1], 
				extradata[sps + 2], 
				extradata[sps + 3], 
				3
			);

			if (_mp4v2_v_track == MP4_INVALID_TRACK_ID) {
				error = AE_MP4V2_ADD_TRACK_FAILED;
				break;
			}

			MP4SetVideoProfileLevel(_mp4v2_file, 1);
			MP4AddH264SequenceParameterSet(_mp4v2_file, _mp4v2_v_track, extradata + sps, pps - sps - 4);
			MP4AddH264PictureParameterSet(_mp4v2_file, _mp4v2_v_track, extradata + pps, extradata_size - pps);
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
					std::bind(&muxer_libmp4v2::on_enc_aac_data, this, std::placeholders::_1, std::placeholders::_2),
					std::bind(&muxer_libmp4v2::on_enc_aac_error, this, std::placeholders::_1)
				);

				SAMPLE_SETTING src_setting = {
					_a_stream->a_enc->get_nb_samples(),
					av_get_default_channel_layout(_a_stream->a_src[i]->get_channel_num()),
					_a_stream->a_src[i]->get_channel_num(),
					AFTypetrans2AVType(_a_stream->a_src[i]->get_fmt()),
					_a_stream->a_src[i]->get_sample_rate()
				};
				SAMPLE_SETTING dst_setting = {
					_a_stream->a_enc->get_nb_samples(),
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
				_a_stream->a_samples[i]->size = av_samples_get_buffer_size(NULL, src_setting.nb_channels, src_setting.nb_samples, src_setting.fmt, 1);
				_a_stream->a_samples[i]->buff = new uint8_t[_a_stream->a_samples[i]->size];
			}

			_a_stream->setting = setting;

			_mp4v2_a_track = MP4AddAudioTrack(_mp4v2_file, setting.a_sample_rate, _a_stream->a_enc->get_nb_samples(), MP4_MPEG4_AUDIO_TYPE);
			if (_mp4v2_a_track == MP4_INVALID_TRACK_ID) {
				error = AE_MP4V2_ADD_TRACK_FAILED;
				break;
			}

			const uint8_t *extradata = _a_stream->a_enc->get_extradata();
			int extradata_size = _a_stream->a_enc->get_extradata_size();

			MP4SetAudioProfileLevel(_mp4v2_file, 2);
			MP4SetTrackESConfiguration(_mp4v2_file, _mp4v2_a_track, extradata, extradata_size);
		} while (0);

		return error;
	}

	int muxer_libmp4v2::open_output(const char * output_file, const MUX_SETTING_T & setting)
	{
		int error = AE_NO;
		int ret = 0;

		do {
			_mp4v2_file = MP4Create(output_file, 0);
			if (_mp4v2_file == MP4_INVALID_FILE_HANDLE) {
				error = AE_MP4V2_CREATE_FAILED;
				break;
			}

			MP4SetTimeScale(_mp4v2_file, 90000);
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
		if (_base_time < 0)
			return 0;

		static uint8_t *tmp_buf = new uint8_t[1024 * 1024 * 8];
		memcpy(tmp_buf, data, len);

		tmp_buf[0] = (len - 4) >> 24;
		tmp_buf[1] = (len - 4) >> 16;
		tmp_buf[2] = (len - 4) >> 8;
		tmp_buf[3] = len - 4;

		int64_t cur_time = av_gettime() / 1000;
		int64_t time_stamp = av_rescale_q_rnd(cur_time - _base_time, { 1,1000 }, {1,90000 }, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

		return MP4WriteSample(_mp4v2_file, _mp4v2_v_track, tmp_buf, len, MP4_INVALID_DURATION, 0, key_frame);
	}

	int muxer_libmp4v2::write_audio(const uint8_t * data, int len)
	{
		int64_t cur_time = av_gettime() / 1000;
		if (_base_time < 0)
			_base_time = cur_time;

		int64_t time_stamp = av_rescale_q_rnd(cur_time - _base_time, { 1,1000 }, { 1,_a_stream->setting.a_sample_rate }, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

		return MP4WriteSample(_mp4v2_file, _mp4v2_a_track, data, len, MP4_INVALID_DURATION, 0, 1);
	}


}