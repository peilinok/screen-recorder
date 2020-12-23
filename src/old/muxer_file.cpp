#include "muxer_file.h"

namespace am {

	muxer_file::muxer_file()
	{
		_on_yuv_data = nullptr;

		_inited = false;
		_running = false;
		_paused = false;

		_have_a = false;
		_have_v = false;

		_preview_enabled = false;

		_output_file = "";
	}


	muxer_file::~muxer_file()
	{
	}

}