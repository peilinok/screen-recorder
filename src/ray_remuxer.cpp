#include "ray_remuxer.h"

#include <mutex>

#include "ffmpeg\common.h"

#include "constants\ray_error_str.h"

#include "utils\ray_log.h"

namespace ray {
namespace remuxer {


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

static ErrorCode open_src(AVFormatContext **ctx, const char *path) {
	int ret = avformat_open_input(ctx, path, NULL, NULL);
	if (ret < 0) {
		return ERR_FFMPEG_OPEN_INPUT_FAILED;
	}

	ret = avformat_find_stream_info(*ctx, NULL);
	if (ret < 0) {
		return ERR_FFMPEG_FIND_STREAM_FAILED;
	}

#ifdef _DEBUG
	av_dump_format(*ctx, 0, path, false);
#endif
	return ERR_NO;
}

static ErrorCode open_dst(AVFormatContext **ctx_dst, const char *path, AVFormatContext *ctx_src) {
	int ret;

	avformat_alloc_output_context2(ctx_dst, NULL, NULL,
		path);
	if (!*ctx_dst) {
		return ERR_FFMPEG_ALLOC_CONTEXT_FAILED;
	}

	for (unsigned i = 0; i < ctx_src->nb_streams; i++) {
		AVStream *in_stream = ctx_src->streams[i];
		AVStream *out_stream = avformat_new_stream(
			*ctx_dst, in_stream->codec->codec);
		if (!out_stream) {
			return ERR_FFMPEG_NEW_STREAM_FAILED;
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
			return ERR_FFMPEG_COPY_PARAMS_FAILED;
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
			return ERR_FFMPEG_OPEN_IO_FAILED;
		}
	}

	return ERR_NO;
}

void RemuxerTask::start()
{
	if (running_)
		return;

	running_ = true;
	thread_ = std::thread(std::bind(&RemuxerTask::task, this));
}

void RemuxerTask::stop()
{
	if (!running_)
		return;

	running_ = false;
	if (thread_.joinable())
		thread_.join();
}

void RemuxerTask::task()
{
	ErrorCode error = ERR_NO;

	AVFormatContext *ctx_src = nullptr, *ctx_dst = nullptr;

	//call back start
	if (event_handler_)
		event_handler_->onRemuxState(src_.c_str(), 1, ERR_NO);

	auto remux_loop = [this](AVFormatContext *ctx_src,
		AVFormatContext *ctx_dst, const std::string& src, const int64_t src_size, IRemuxerEventHandler *handler)
	{
		AVPacket pkt;

		int ret, throttle = 0;
		ErrorCode error = ERR_NO;

		do {
			ret = av_read_frame(ctx_src, &pkt);
			if (ret < 0) {
				if (ret != AVERROR_EOF)
					error = ERR_FFMPEG_READ_FRAME_FAILED;
				break;
			}

			if (handler && throttle++ > 10) {
				float progress = pkt.pos / (float)src_size * 100.f;
				handler->onRemuxProgress(src.c_str(), progress, 100);
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
				error = ERR_FFMPEG_WRITE_FRAME_FAILED;
				break;
			}
		} while (this->isRunning());

		return error;
	};

	do {
		error = open_src(&ctx_src, src_.c_str());
		if (error != ERR_NO) {
			break;
		}

		error = open_dst(&ctx_dst, dst_.c_str(), ctx_src);
		if (error != ERR_NO) {
			break;
		}

		int ret = avformat_write_header(ctx_dst, NULL);
		if (ret < 0) {
			error = ERR_FFMPEG_WRITE_HEADER_FAILED;
			break;
		}

		error = remux_loop(ctx_src, ctx_dst, src_, src_size_, event_handler_);
		if (error != ERR_NO) {
			av_write_trailer(ctx_dst);
			break;
		}

		ret = av_write_trailer(ctx_dst);
		if (ret < 0)
			error = ERR_FFMPEG_WRITE_TRAILER_FAILED;


	} while (0);


	if (ctx_src) {
		avformat_close_input(&ctx_src);
	}

	if (ctx_dst && !(ctx_dst->oformat->flags & AVFMT_NOFILE))
		avio_close(ctx_dst->pb);

	if (ctx_dst)
		avformat_free_context(ctx_dst);

	LOG(INFO) << "remux from \"" << src_ << "\" to \"" << dst_ << "\" end with error: " << (err2str(error));

	//call back end
	if (event_handler_)
		event_handler_->onRemuxState(src_.c_str(), 0, error);

	running_ = false;

	//Remuxer::getInstance()->stop(src_.c_str());
}



rt_error Remuxer::initialize(const RemuxerConfiguration & config)
{
	return rt_error();
}

void Remuxer::release()
{
	stopAll();
}

void Remuxer::setEventHandler(IRemuxerEventHandler * handler)
{
	event_handler_ = handler;
}

rt_error Remuxer::remux(const char* srcFilePath, const char* dstFilePath) {

	std::lock_guard<std::mutex> lock(mutex_);

	auto itr = tasks_.find(srcFilePath);
	if (itr != tasks_.end() && itr->second->isRunning()) {
		return ERR_REMUX_RUNNING;
	}

	if (!strlen(srcFilePath) || !strlen(dstFilePath) || !strcmp(srcFilePath, dstFilePath))
		return ERR_REMUX_INVALID_INOUT;

#ifdef _MSC_VER
	struct _stat64 st = { 0 };
	_stat64(srcFilePath, &st);
#else
	struct stat st = { 0 };
	stat(param.src, &st);
#endif

	// should delete this later, after use threadpool
	if (itr != tasks_.end())
		tasks_.erase(itr);

	if (!st.st_size) return ERR_REMUX_NOT_EXIST;

	std::auto_ptr<RemuxerTask> task;
	task.reset(new RemuxerTask(srcFilePath, st.st_size, dstFilePath, this));

	tasks_[srcFilePath] = task;
	tasks_[srcFilePath]->start();

	return ERR_NO;
}

void Remuxer::stop(const char* srcFilePath)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto itr = tasks_.find(srcFilePath);
	if (itr == tasks_.end())
		return;

	itr->second.reset(nullptr);
	tasks_.erase(itr);
}

void Remuxer::stopAll()
{
	std::lock_guard<std::mutex> lock(mutex_);

	tasks_.clear();
}

void Remuxer::onRemuxProgress(const char * src, uint8_t progress, uint8_t total)
{
	if (event_handler_)
		event_handler_->onRemuxProgress(src, progress, total);
}

void Remuxer::onRemuxState(const char * src, bool succeed, rt_error error)
{
	if (event_handler_)
		event_handler_->onRemuxState(src, succeed, error);
}

}
}