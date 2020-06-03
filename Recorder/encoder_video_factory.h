#ifndef ENCODER_VIDEO_FACTORY
#define ENCODER_VIDEO_FACTORY

#include "encoder_video_define.h"

namespace am {
	class encoder_video;

	int encoder_video_new(ENCODER_VIDEO_ID id, encoder_video **encoder);

	void encoder_video_destroy(encoder_video **encoder);
}

#endif // !ENCODER_VIDEO_FACTORY

