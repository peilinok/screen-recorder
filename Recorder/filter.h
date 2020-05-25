#ifndef FILTER
#define FILTER

#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>

#include "headers_ffmpeg.h"

namespace am {

	typedef struct {
		AVFilterContext *ctx;
		AVFilterInOut *inout;

		AVRational time_base;
		int sample_rate;
		AVSampleFormat sample_fmt;
		int nb_channel;
		int64_t channel_layout;
	}FILTER_CTX;

	typedef std::function<void(AVFrame *)> on_filter_data;
	typedef std::function<void(int)> on_filter_error;

	void format_pad_arg(char *arg, int size, const FILTER_CTX &ctx);
}

#endif // !FILTER

