#include "stdafx.h"
#include "muxer_mp4.h"

namespace am {

	muxer_mp4::muxer_mp4()
	{
	}

	muxer_mp4::~muxer_mp4()
	{
	}

	int muxer_mp4::init(
		const char * output_file,
		const record_desktop * source_desktop,
		const record_audio ** source_audios,
		const MUX_SETTING & setting
	)
	{
		return 0;
	}

	void muxer_mp4::release()
	{
	}

	int muxer_mp4::start()
	{
		return 0;
	}

	int muxer_mp4::stop()
	{
		return 0;
	}

	int muxer_mp4::pause()
	{
		return 0;
	}

	int muxer_mp4::resume()
	{
		return 0;
	}

}