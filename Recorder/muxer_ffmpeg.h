#ifndef MUXER_FFMPEG
#define MUXER_FFMPEG

#include <thread>
#include <list>
#include <functional>
#include <math.h>
#include <mutex>

#include "muxer_file.h"

#include "headers_ffmpeg.h"

namespace am {

	class muxer_ffmpeg : public muxer_file
	{
	public:
		muxer_ffmpeg();
		~muxer_ffmpeg();

		int init(
			const char *output_file,
			record_desktop *source_desktop,
			record_audio ** source_audios,
			const int source_audios_nb,
			const MUX_SETTING_T &setting
		);

		int start();
		int stop();

		int pause();
		int resume();

	private:
		void on_desktop_data(AVFrame *frame);

		void on_desktop_error(int error);

		void on_audio_data(AVFrame *frame, int index);

		void on_audio_error(int error, int index);

		void on_filter_amix_data(AVFrame *frame, int index);

		void on_filter_amix_error(int error, int index);

		void on_filter_aresample_data(AVFrame * frame,int index);

		void on_filter_aresample_error(int error, int index);



		void on_enc_264_data(AVPacket *packet);

		void on_enc_264_error(int error);

		void on_enc_aac_data(AVPacket *packet);

		void on_enc_aac_error(int error);



		int alloc_oc(const char *output_file, const MUX_SETTING_T &setting);

		int add_video_stream(const MUX_SETTING_T &setting, record_desktop *source_desktop);

		int add_audio_stream(const MUX_SETTING_T &setting, record_audio ** source_audios, const int source_audios_nb);

		int open_output(const char *output_file, const MUX_SETTING_T &setting);

		void cleanup_video();
		void cleanup_audio();
		void cleanup();

		uint64_t get_current_time();

		int write_video(AVPacket *packet);

		int write_audio(AVPacket *packet);

	private:
		struct MUX_STREAM_T *_v_stream, *_a_stream;

		AVOutputFormat *_fmt;
		AVFormatContext *_fmt_ctx;

		int64_t _base_time;

		char ff_error[4096];

		std::mutex _mutex;
		std::mutex _time_mutex;
	};
}



#endif
