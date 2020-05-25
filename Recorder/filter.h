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

	/**
	* filter data callback
	* @param frame pointer to a AVFrame
	* @param index resource index ,default is -1
	*/
	typedef std::function<void(AVFrame *frame, int index)> on_filter_data;

	/**
	* filter error callback
	* @param code error code
	* @param index resource index ,default is -1
	*/
	typedef std::function<void(int code, int index)> on_filter_error;

	void format_pad_arg(char *arg, int size, const FILTER_CTX &ctx);
}

#endif // !FILTER

