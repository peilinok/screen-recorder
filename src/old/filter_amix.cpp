#include "filter_amix.h"

#include <chrono>

#include "error_define.h"

#include "utils\ray_log.h"

namespace am {

	static void print_frame(const AVFrame *frame, int index)
	{
		VLOG(VLOG_DEBUG) << "index: " << index << " pts: " << frame->pts << " nb: " << frame->nb_samples;
	}

	filter_amix::filter_amix()
	{
		av_register_all();
		avfilter_register_all();

		memset(&_ctx_in_0, 0, sizeof(FILTER_CTX));
		memset(&_ctx_in_1, 0, sizeof(FILTER_CTX));
		memset(&_ctx_out, 0, sizeof(FILTER_CTX));

		_filter_graph = NULL;

		_inited = false;
		_running = false;

		_cond_notify = false;

	}


	filter_amix::~filter_amix()
	{
		stop();
		cleanup();
	}

	int filter_amix::init(const FILTER_CTX & ctx_in0, const FILTER_CTX & ctx_in1, const FILTER_CTX & ctx_out)
	{
		int error = AE_NO;
		int ret = 0;

		if (_inited) return AE_NO;

		do {
			_ctx_in_0 = ctx_in0;
			_ctx_in_1 = ctx_in1;
			_ctx_out = ctx_out;

			_filter_graph = avfilter_graph_alloc();
			if (!_filter_graph) {
				error = AE_FILTER_ALLOC_GRAPH_FAILED;
				break;
			}

			const std::string filter_desrc = "[in0][in1]amix=inputs=2:duration=first:dropout_transition=0[out]";

			_ctx_in_0.inout = avfilter_inout_alloc();
			_ctx_in_1.inout = avfilter_inout_alloc();
			_ctx_out.inout = avfilter_inout_alloc();

			char pad_args0[512] = { 0 }, pad_args1[512] = { 0 };

			format_pad_arg(pad_args0, 512, _ctx_in_0);
			format_pad_arg(pad_args1, 512, _ctx_in_1);

			ret = avfilter_graph_create_filter(&_ctx_in_0.ctx, avfilter_get_by_name("abuffer"), "in0", pad_args0, NULL, _filter_graph);
			if (ret < 0) {
				error = AE_FILTER_CREATE_FILTER_FAILED;
				break;
			}

			ret = avfilter_graph_create_filter(&_ctx_in_1.ctx, avfilter_get_by_name("abuffer"), "in1", pad_args1, NULL, _filter_graph);
			if (ret < 0) {
				error = AE_FILTER_CREATE_FILTER_FAILED;
				break;
			}

			ret = avfilter_graph_create_filter(&_ctx_out.ctx, avfilter_get_by_name("abuffersink"), "out", NULL, NULL, _filter_graph);
			if (ret < 0) {
				error = AE_FILTER_CREATE_FILTER_FAILED;
				break;
			}

			av_opt_set_bin(_ctx_out.ctx, "sample_fmts", (uint8_t*)&_ctx_out.sample_fmt, sizeof(_ctx_out.sample_fmt), AV_OPT_SEARCH_CHILDREN);
			av_opt_set_bin(_ctx_out.ctx, "channel_layouts", (uint8_t*)&_ctx_out.channel_layout, sizeof(_ctx_out.channel_layout), AV_OPT_SEARCH_CHILDREN);
			av_opt_set_bin(_ctx_out.ctx, "sample_rates", (uint8_t*)&_ctx_out.sample_rate, sizeof(_ctx_out.sample_rate), AV_OPT_SEARCH_CHILDREN);

			_ctx_in_0.inout->name = av_strdup("in0");
			_ctx_in_0.inout->filter_ctx = _ctx_in_0.ctx;
			_ctx_in_0.inout->pad_idx = 0;
			_ctx_in_0.inout->next = _ctx_in_1.inout;

			_ctx_in_1.inout->name = av_strdup("in1");
			_ctx_in_1.inout->filter_ctx = _ctx_in_1.ctx;
			_ctx_in_1.inout->pad_idx = 0;
			_ctx_in_1.inout->next = NULL;

			_ctx_out.inout->name = av_strdup("out");
			_ctx_out.inout->filter_ctx = _ctx_out.ctx;
			_ctx_out.inout->pad_idx = 0;
			_ctx_out.inout->next = NULL;

			AVFilterInOut *inoutputs[2] = { _ctx_in_0.inout,_ctx_in_1.inout };

			ret = avfilter_graph_parse_ptr(_filter_graph, filter_desrc.c_str(), &_ctx_out.inout, inoutputs, NULL);
			if (ret < 0) {
				error = AE_FILTER_PARSE_PTR_FAILED;
				break;
			}

			ret = avfilter_graph_config(_filter_graph, NULL);
			if (ret < 0) {
				error = AE_FILTER_CONFIG_FAILED;
				break;
			}

			//al_debug("dump graph:\r\n%s", avfilter_graph_dump(_filter_graph, NULL));

			_inited = true;
		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "filter amix init failed: " << (err2str(error)) << " ,ret: " << ret;
			cleanup();
		}

		//if (_ctx_in_0.inout)
		//	avfilter_inout_free(&_ctx_in_0.inout);

		//if (_ctx_in_1.inout)
		//	avfilter_inout_free(&_ctx_in_1.inout);

		//if (_ctx_out.inout)
		//	avfilter_inout_free(&_ctx_out.inout);

		return error;
	}

	int filter_amix::start()
	{
		if (!_inited)
			return AE_NEED_INIT;

		if (_running)
			return AE_NO;

		_running = true;
		_thread = std::thread(std::bind(&filter_amix::filter_loop, this));

		return 0;
	}

	int filter_amix::stop()
	{
		if (!_inited || !_running)
			return AE_NO;

		_running = false;

		_cond_notify = true;
		_cond_var.notify_all();

		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	int filter_amix::add_frame(AVFrame * frame, int index)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		int error = AE_NO;
		int ret = 0;

		do {
			AVFilterContext *ctx = NULL;
			switch (index) {
			case 0:
				ctx = _ctx_in_0.ctx;
				break;
			case 1:
				ctx = _ctx_in_1.ctx;
				break;
			default:
				ctx = NULL;
				break;
			}

			if (!ctx) {
				error = AE_FILTER_INVALID_CTX_INDEX;
				break;
			}

			//print_frame(frame, index);
			int ret = av_buffersrc_add_frame_flags(ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
			if (ret < 0) {
				error = AE_FILTER_ADD_FRAME_FAILED;
				break;
			}

		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "filter amix add frame failed: " << (err2str(error)) << " ,ret: " << ret;
		}

		_cond_notify = true;
		_cond_var.notify_all();

		return error;
	}

	const AVRational filter_amix::get_time_base()
	{
		return av_buffersink_get_time_base(_ctx_out.ctx);
	}

	void filter_amix::cleanup()
	{
		if (_filter_graph)
			avfilter_graph_free(&_filter_graph);

		memset(&_ctx_in_0, 0, sizeof(FILTER_CTX));
		memset(&_ctx_in_1, 0, sizeof(FILTER_CTX));
		memset(&_ctx_out, 0, sizeof(FILTER_CTX));

		_inited = false;
	}

	void filter_amix::filter_loop()
	{
		AVFrame *frame = av_frame_alloc();

		int ret = 0;
		while (_running) {
			std::unique_lock<std::mutex> lock(_mutex);
			while (!_cond_notify && _running)
				_cond_var.wait_for(lock,std::chrono::milliseconds(300));

			while (_running && _cond_notify) {
				ret = av_buffersink_get_frame(_ctx_out.ctx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;;
				}

				if (ret < 0) {
					LOG(ERROR) << "filter amix get frame error: " << ret;
					if (_on_filter_error) _on_filter_error(ret, -1);
					break;
				}

				if (_on_filter_data)
					_on_filter_data(frame, -1);

				av_frame_unref(frame);
			}

			_cond_notify = false;
		}

		av_frame_free(&frame);
	}

}