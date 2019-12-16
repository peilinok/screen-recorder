#ifndef FILTER_AUDIO
#define FILTER_AUDIO

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

	class filter_audio
	{
	public:
		filter_audio();
		~filter_audio();

		int init(const FILTER_CTX &ctx_in0, const FILTER_CTX &ctx_in1, const FILTER_CTX &ctx_out);

		inline void registe_cb(on_filter_data cb_on_filter_data, on_filter_error cb_on_filter_error) {
			_on_filter_data = cb_on_filter_data;
			_on_filter_error = cb_on_filter_error;
		}

		int start();

		int stop();

		int add_frame(AVFrame *frame, int index);

		const AVRational &get_time_base();

	private:
		void cleanup();
		void filter_loop();



	private:
		FILTER_CTX _ctx_in_0;
		FILTER_CTX _ctx_in_1;
		FILTER_CTX _ctx_out;

		AVFilterGraph *_filter_graph;

		on_filter_data _on_filter_data;
		on_filter_error _on_filter_error;

		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		std::mutex _mutex;
		std::condition_variable _cond_var;
		bool _cond_notify;
	};

}

#endif