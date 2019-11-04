#ifndef MUXER_MP4
#define MUXER_MP4

#include <atomic>
#include <thread>
#include <list>
#include <functional>

namespace am {
	class record_audio;
	class record_desktop;
	class encoder_264;
	class resample_pcm;
	class filter_amix;

	typedef struct {

	}MUX_SETTING;

	class muxer_mp4 {
	public:
		muxer_mp4();
		~muxer_mp4();

		int init(
			const char *output_file,
			const record_desktop *source_desktop,
			const record_audio ** source_audios,
			const MUX_SETTING &setting
		);
		void release();

		int start();
		int stop();

		int pause();
		int resume();

	protected:
		record_desktop *_desktop_source;
		record_audio **_audio_sources;
		resample_pcm **_pcm_resampler;
		filter_amix *filter_audio;

	private:

	};
}



#endif
