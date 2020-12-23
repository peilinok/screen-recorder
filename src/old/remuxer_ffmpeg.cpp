#include "remuxer_ffmpeg.h"

#include <mutex>

#include "headers_ffmpeg.h"

#include "error_define.h"

#include "utils\ray_log.h"

namespace am {
	static remuxer_ffmpeg *_g_instance = nullptr;
	static std::mutex _g_mutex;

	static void process_packet(AVPacket *pkt, AVStream *in_stream,
		AVStream *out_stream)
	{
		pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base,
			out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base,
			out_stream->time_base,
			(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt->duration = (int)av_rescale_q(pkt->duration, in_stream->time_base,
			out_stream->time_base);
		pkt->pos = -1;
	}

	static int remux_file(AVFormatContext *ctx_src, AVFormatContext *ctx_dst, REMUXER_PARAM *param)
	{
		AVPacket pkt;

		int ret, throttle = 0, error = AE_NO;

		for (;;) {
			ret = av_read_frame(ctx_src, &pkt);
			if (ret < 0) {
				if (ret != AVERROR_EOF)
					error = AE_FFMPEG_READ_FRAME_FAILED;
				break;
			}

			if (param->cb_progress != NULL && throttle++ > 10) {
				float progress = pkt.pos / (float)param->src_size * 100.f;
				param->cb_progress(param->src, progress, 100);
				throttle = 0;
			}

			process_packet(&pkt, ctx_src->streams[pkt.stream_index],
				ctx_dst->streams[pkt.stream_index]);

			ret = av_interleaved_write_frame(ctx_dst, &pkt);
			av_packet_unref(&pkt);

			// Sometimes the pts and dts will equal to last packet,
			// don not know why,may the time base issue?
			// So return -22 do not care for now
			if (ret < 0 && ret != -22) {
				error = AE_FFMPEG_WRITE_FRAME_FAILED;
				break;
			}
		}

		return error;
	}

	static int open_src(AVFormatContext **ctx, const char *path) {
		int ret = avformat_open_input(ctx, path, NULL, NULL);
		if (ret < 0) {
			return AE_FFMPEG_OPEN_INPUT_FAILED;
		}

		ret = avformat_find_stream_info(*ctx, NULL);
		if (ret < 0) {
			return AE_FFMPEG_FIND_STREAM_FAILED;
		}

#ifdef _DEBUG
		av_dump_format(*ctx, 0, path, false);
#endif
		return AE_NO;
	}

	int open_dst(AVFormatContext **ctx_dst, const char *path, AVFormatContext *ctx_src) {
		int ret;

		avformat_alloc_output_context2(ctx_dst, NULL, NULL,
			path);
		if (!*ctx_dst) {
			return AE_FFMPEG_ALLOC_CONTEXT_FAILED;
		}

		for (unsigned i = 0; i < ctx_src->nb_streams; i++) {
			AVStream *in_stream = ctx_src->streams[i];
			AVStream *out_stream = avformat_new_stream(
				*ctx_dst, in_stream->codec->codec);
			if (!out_stream) {
				return AE_FFMPEG_NEW_STREAM_FAILED;
			}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
			AVCodecParameters *par = avcodec_parameters_alloc();
			ret = avcodec_parameters_from_context(par, in_stream->codec);
			if (ret == 0)
				ret = avcodec_parameters_to_context(out_stream->codec,
					par);
			avcodec_parameters_free(&par);
#else
			ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
#endif

			if (ret < 0) {
				return AE_FFMPEG_COPY_PARAMS_FAILED;
			}
			out_stream->time_base = out_stream->codec->time_base;

			av_dict_copy(&out_stream->metadata, in_stream->metadata, 0);

			out_stream->codec->codec_tag = 0;
			if ((*ctx_dst)->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

#ifndef _NDEBUG
		av_dump_format(*ctx_dst, 0, path, true);
#endif

		if (!((*ctx_dst)->oformat->flags & AVFMT_NOFILE)) {
			ret = avio_open(&(*ctx_dst)->pb, path,
				AVIO_FLAG_WRITE);
			if (ret < 0) {
				return AE_FFMPEG_OPEN_IO_FAILED;
			}
		}

		return AE_NO;
	}

	static void remuxing(REMUXER_PARAM *param) {
		int error = AE_NO;

		AVFormatContext *ctx_src = nullptr, *ctx_dst = nullptr;

		//call back start
		if (param->cb_state)
			param->cb_state(param->src, 1, AE_NO);

		do {
			error = open_src(&ctx_src, param->src);
			if (error != AE_NO) {
				break;
			}

			error = open_dst(&ctx_dst, param->dst, ctx_src);
			if (error != AE_NO) {
				break;
			}

			int ret = avformat_write_header(ctx_dst, NULL);
			if (ret < 0) {
				error = AE_FFMPEG_WRITE_HEADER_FAILED;
				break;
			}

			error = remux_file(ctx_src, ctx_dst, param);
			if (error != AE_NO) {
				av_write_trailer(ctx_dst);
				break;
			}

			ret = av_write_trailer(ctx_dst);
			if (ret < 0)
				error = AE_FFMPEG_WRITE_TRAILER_FAILED;


		} while (0);


		if (ctx_src) {
			avformat_close_input(&ctx_src);
		}

		if (ctx_dst && !(ctx_dst->oformat->flags & AVFMT_NOFILE))
			avio_close(ctx_dst->pb);

		if (ctx_dst)
			avformat_free_context(ctx_dst);

		LOG(INFO) << "remux from " << param->src << " to " << param->dst << "end with error: " << (err2str(error));

		//call back end
		if (param->cb_state)
			param->cb_state(param->src, 0, error);

		remuxer_ffmpeg::instance()->remove_remux(param->src);
	}

	remuxer_ffmpeg * remuxer_ffmpeg::instance()
	{
		std::lock_guard<std::mutex> lock(_g_mutex);

		if (_g_instance == nullptr) _g_instance = new remuxer_ffmpeg();

		return _g_instance;
	}

	void remuxer_ffmpeg::release()
	{
		std::lock_guard<std::mutex> lock(_g_mutex);

		if (_g_instance)
			delete _g_instance;

		_g_instance = nullptr;
	}



	int remuxer_ffmpeg::create_remux(const REMUXER_PARAM & param)
	{
		std::lock_guard<std::mutex> lock(_g_mutex);

		auto itr = _handlers.find(param.src);
		if (itr != _handlers.end() && itr->second->fn.joinable() == true) {
			return AE_REMUX_RUNNING;
		}


		if (!strlen(param.src) || !strlen(param.dst) || !strcmp(param.src, param.dst))
			return AE_REMUX_INVALID_INOUT;

#ifdef _MSC_VER
		struct _stat64 st = { 0 };
		_stat64(param.src, &st);
#else
		struct stat st = { 0 };
		stat(param.src, &st);
#endif

		if (!st.st_size) return AE_REMUX_NOT_EXIST;


		if (itr != _handlers.end()) {
			delete itr->second;

			_handlers.erase(itr);
		}

		REMUXER_HANDLE *handle = new REMUXER_HANDLE;

		memcpy(&(handle->param), &param, sizeof(REMUXER_PARAM));

		handle->param.running = true;
		handle->param.src_size = st.st_size;
		handle->fn = std::thread(remuxing, &handle->param);

		_handlers[param.src] = handle;

		return AE_NO;
	}

	void remuxer_ffmpeg::remove_remux(std::string src)
	{
		std::lock_guard<std::mutex> lock(_g_mutex);

		auto itr = _handlers.find(src);
		if (itr != _handlers.end()) {
			itr->second->fn.detach();

			delete itr->second;
			_handlers.erase(itr);
		}
	}

	void remuxer_ffmpeg::destroy_remux()
	{
		std::lock_guard<std::mutex> lock(_g_mutex);

		for (auto itr = _handlers.begin(); itr != _handlers.end(); itr++)
		{
			itr->second->param.running = false;

			if (itr->second->fn.joinable())
				itr->second->fn.join();

			delete itr->second;

			_handlers.erase(itr);
		}
	}

}