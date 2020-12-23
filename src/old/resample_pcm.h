#ifndef RESAMPLE_PCM
#define RESAMPLE_PCM

#include <stdint.h>

#include "headers_ffmpeg.h"

namespace am {
	typedef struct {
		int nb_samples;
		int64_t channel_layout;
		int nb_channels;
		AVSampleFormat fmt;
		int sample_rate;
	}SAMPLE_SETTING;

	class resample_pcm
	{
	public:
		resample_pcm();
		~resample_pcm();

		int init(const SAMPLE_SETTING *sample_src, const SAMPLE_SETTING *sample_dst,__out int *resapmled_frame_size);
		int convert(const uint8_t *src, int src_len, uint8_t *dst, int dst_len);
	protected:
		void cleanup();
	private:
		SwrContext *_ctx;
		SAMPLE_SETTING *_sample_src;
		SAMPLE_SETTING *_sample_dst;
	};
}
#endif
