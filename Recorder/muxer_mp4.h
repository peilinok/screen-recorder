#ifndef MUXER_MP4
#define MUXER_MP4

#include <atomic>
#include <thread>
#include <list>
#include <functional>
#include <math.h>

extern "C" {
#include <libavformat/avformat.h>
}

namespace am {
	class record_audio;
	class record_desktop;

	struct MUX_STREAM_T;


	typedef struct {
		int v_frame_rate;
		int v_bit_rate;

		int a_nb_samples;
		int a_nb_channel;
		int a_sample_rate;
		AVSampleFormat a_sample_fmt;
		int a_bit_rate;
	}MUX_SETTING;

	class muxer_mp4 {
	public:
		muxer_mp4();
		~muxer_mp4();

		int init(
			const char *output_file,
			record_desktop *source_desktop,
			record_audio ** source_audios,
			const int source_audios_nb,
			const MUX_SETTING &setting
		);

		int start();
		int stop();

		int pause();
		int resume();

	private:
		void on_desktop_data(const uint8_t *data, int len);

		void on_desktop_error(int error);

		void on_audio_data(const uint8_t *data, int len, int index);

		void on_audio_error(int error, int index);

		void on_enc_264_data(const uint8_t *data, int len);

		void on_enc_264_error(int error);

		void on_enc_aac_data(const uint8_t *data, int len);

		void on_enc_aac_error(int error);

		int alloc_oc(const char *output_file, const MUX_SETTING &setting);

		int add_video_stream(const MUX_SETTING &setting, record_desktop *source_desktop);

		int add_audio_stream(const MUX_SETTING &setting, record_audio ** source_audios, const int source_audios_nb);

		int open_output(const char *output_file, const MUX_SETTING &setting);

		void cleanup_video();
		void cleanup_audio();
		void cleanup();

		int get_video_data();

		int get_audio_data();

		int write_video();

		int write_audio();

		void mux_loop();

	private:
		std::atomic_bool _inited;
		std::atomic_bool _running;

		std::thread _thread;

		bool _have_v, _have_a;

		std::string _output_file;

		struct MUX_STREAM_T *_v_stream, *_a_stream;
		AVOutputFormat *_fmt;
		AVFormatContext *_fmt_ctx;

		char ff_error[4096];
	};
}



#endif
