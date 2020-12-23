#include "resample_pcm.h"

#include "error_define.h"

#include "utils\ray_log.h"

namespace am {
	resample_pcm::resample_pcm()
	{
		_sample_src = NULL;
		_sample_dst = NULL;
		_ctx = NULL;
	}

	resample_pcm::~resample_pcm()
	{
		cleanup();
	}

	int resample_pcm::init(const SAMPLE_SETTING * sample_src, const SAMPLE_SETTING * sample_dst, int * resapmled_frame_size)
	{
		int err = AE_NO;

		do {
			_sample_src = (SAMPLE_SETTING*)malloc(sizeof(SAMPLE_SETTING));
			_sample_dst = (SAMPLE_SETTING*)malloc(sizeof(SAMPLE_SETTING));

			memcpy(_sample_src, sample_src, sizeof(SAMPLE_SETTING));
			memcpy(_sample_dst, sample_dst, sizeof(SAMPLE_SETTING));

			_ctx = swr_alloc_set_opts(NULL,
				_sample_dst->channel_layout, _sample_dst->fmt, _sample_dst->sample_rate,
				_sample_src->channel_layout, _sample_src->fmt, _sample_src->sample_rate,
				0, NULL);

			if (_ctx == NULL) {
				err = AE_RESAMPLE_INIT_FAILED;
				break;
			}

			int ret = swr_init(_ctx);
			if (ret < 0) {
				err = AE_RESAMPLE_INIT_FAILED;
				break;
			}



			*resapmled_frame_size = av_samples_get_buffer_size(NULL, _sample_dst->nb_channels, _sample_dst->nb_samples, _sample_dst->fmt, 1);

		} while (0);

		if (err != AE_NO) {
			cleanup();
			LOG(ERROR) << "resample pcm init failed: " << (err2str(err));
		}

		return err;
	}

	int resample_pcm::convert(const uint8_t * src, int src_len, uint8_t * dst, int dst_len)
	{

		uint8_t *out[2] = { 0 };
		out[0] = dst;
		out[1] = dst + dst_len / 2;

		const uint8_t *in1[2] = { src,NULL };

		/*
		uint8_t *in[2] = { 0 };
		in[0] = (uint8_t*)src;
		in[1] = (uint8_t*)(src + src_len / 2);

		AVFrame *sample_frame = av_frame_alloc();
		sample_frame->nb_samples = 1024;
		sample_frame->channel_layout = _sample_dst->channel_layout;
		sample_frame->format = _sample_dst->fmt;
		sample_frame->sample_rate = _sample_dst->sample_rate;

		avcodec_fill_audio_frame(sample_frame, _sample_dst->nb_channels, _sample_dst->fmt, src, src_len, 0);
		*/

		return swr_convert(_ctx, out, _sample_dst->nb_samples, in1, _sample_src->nb_samples);
	}
	void resample_pcm::cleanup()
	{
		if(_sample_src)
			free(_sample_src);

		if(_sample_dst)
			free(_sample_dst);

		if(_ctx)
			swr_free(&_ctx);
	}
}