#include "filter_aresample.h"

#include <chrono>
#include <sstream>

#include "error_define.h"

#include "utils\ray_log.h"

namespace am {

	filter_aresample::filter_aresample()
	{
		av_register_all();
		avfilter_register_all();

		memset(&_ctx_in, 0, sizeof(FILTER_CTX));
		memset(&_ctx_out, 0, sizeof(FILTER_CTX));

		_filter_graph = NULL;

		_inited = false;
		_running = false;

		_cond_notify = false;

		_index = -1;
	}


	filter_aresample::~filter_aresample()
	{
		stop();
		cleanup();
	}

	int filter_aresample::init(const FILTER_CTX & ctx_in, const FILTER_CTX & ctx_out, int index)
	{
		int error = AE_NO;
		int ret = 0;

		if (_inited) return AE_NO;

		_index = index;

		do {
			_ctx_in = ctx_in;
			_ctx_out = ctx_out;

			_filter_graph = avfilter_graph_alloc();
			if (!_filter_graph) {
				error = AE_FILTER_ALLOC_GRAPH_FAILED;
				break;
			}

			char layout_name[256] = { 0 };
			av_get_channel_layout_string(layout_name, 256, ctx_out.nb_channel, ctx_out.channel_layout);


			std::stringstream filter_desrcss;
			filter_desrcss << "aresample=";
			filter_desrcss << ctx_out.sample_rate;
			filter_desrcss << ",aformat=sample_fmts=";
			filter_desrcss << av_get_sample_fmt_name(ctx_out.sample_fmt);
			filter_desrcss << ":channel_layouts=";
			filter_desrcss << layout_name;

			std::string filter_desrc = filter_desrcss.str();

			_ctx_in.inout = avfilter_inout_alloc();
			_ctx_out.inout = avfilter_inout_alloc();

			char pad_args[512] = { 0 };

			format_pad_arg(pad_args, 512, _ctx_in);

			ret = avfilter_graph_create_filter(&_ctx_in.ctx, avfilter_get_by_name("abuffer"), "in", pad_args, NULL, _filter_graph);
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

			_ctx_in.inout->name = av_strdup("in");
			_ctx_in.inout->filter_ctx = _ctx_in.ctx;
			_ctx_in.inout->pad_idx = 0;
			_ctx_in.inout->next = NULL;

			_ctx_out.inout->name = av_strdup("out");
			_ctx_out.inout->filter_ctx = _ctx_out.ctx;
			_ctx_out.inout->pad_idx = 0;
			_ctx_out.inout->next = NULL;

			ret = avfilter_graph_parse_ptr(_filter_graph, filter_desrc.c_str(), &_ctx_out.inout, &_ctx_in.inout, NULL);
			if (ret < 0) {
				error = AE_FILTER_PARSE_PTR_FAILED;
				break;
			}

			ret = avfilter_graph_config(_filter_graph, NULL);
			if (ret < 0) {
				error = AE_FILTER_CONFIG_FAILED;
				break;
			}

			_inited = true;
		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "filter aresample init failed:" << (err2str(error)) << " ret: " << ret;
			cleanup();
		}

		return error;
	}

	int filter_aresample::start()
	{
		if (!_inited)
			return AE_NEED_INIT;

		if (_running)
			return AE_NO;

		_running = true;
		_thread = std::thread(std::bind(&filter_aresample::filter_loop, this));

		return 0;
	}

	int filter_aresample::stop()
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

	int filter_aresample::add_frame(AVFrame * frame)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		int error = AE_NO;
		int ret = 0;

		do {
			int ret = av_buffersrc_add_frame_flags(_ctx_in.ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
			if (ret < 0) {
				error = AE_FILTER_ADD_FRAME_FAILED;
				break;
			}

		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "filter aresample add frame failed: " << (err2str(error)) << " ,ret:" << ret;
		}

		_cond_notify = true;
		_cond_var.notify_all();

		return error;
	}

	const AVRational filter_aresample::get_time_base()
	{
		return av_buffersink_get_time_base(_ctx_out.ctx);
	}

	void filter_aresample::cleanup()
	{
		if (_filter_graph)
			avfilter_graph_free(&_filter_graph);

		memset(&_ctx_in, 0, sizeof(FILTER_CTX));
		memset(&_ctx_out, 0, sizeof(FILTER_CTX));

		_inited = false;
	}

	void filter_aresample::filter_loop()
	{
		AVFrame *frame = av_frame_alloc();

		int ret = 0;
		while (_running) {
			std::unique_lock<std::mutex> lock(_mutex);
			while (!_cond_notify && _running)
				_cond_var.wait_for(lock, std::chrono::milliseconds(300));

			while (_running && _cond_notify) {
				ret = av_buffersink_get_frame(_ctx_out.ctx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;;
				}

				if (ret < 0) {
					LOG(ERROR) << "filter aresample get frame error: " << ret;
					if (_on_filter_error) _on_filter_error(ret, _index);
					break;
				}

				if (_on_filter_data)
					_on_filter_data(frame, _index);

				av_frame_unref(frame);
			}

			_cond_notify = false;
		}

		av_frame_free(&frame);
	}

}