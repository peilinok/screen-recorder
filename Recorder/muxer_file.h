#pragma once

namespace am {

	class record_audio;
	class record_desktop;

	struct MUX_STREAM_T;
	struct MUX_SETTING_T;

	class muxer_file
	{
	public:
		muxer_file();
		virtual ~muxer_file();

		virtual int init(
			const char *output_file,
			record_desktop *source_desktop,
			record_audio ** source_audios,
			const int source_audios_nb,
			const MUX_SETTING_T &setting
		) = 0;

		virtual int start() = 0;
		virtual int stop() = 0;

		virtual int pause() = 0;
		virtual int resume() = 0;
	};


}