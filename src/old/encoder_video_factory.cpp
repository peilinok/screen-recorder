#include "encoder_video_factory.h"

#include "encoder_video.h"
#include "encoder_video_x264.h"
#include "encoder_video_nvenc.h"

#include "error_define.h"

namespace am {

	int encoder_video_new(ENCODER_VIDEO_ID id, encoder_video ** encoder)
	{
		int err = AE_NO;

		switch (id)
		{
		case EID_VIDEO_X264:
			*encoder = (encoder_video*)new encoder_video_x264();
			break;
		case EID_VIDEO_NVENC:
			*encoder = (encoder_video*)new encoder_video_nvenc();
			break;
		default:
			err = AE_UNSUPPORT;
			break;
		}

		return err;
	}

	void encoder_video_destroy(encoder_video ** encoder)
	{
		if (*encoder != nullptr) {
			(*encoder)->stop();

			delete *encoder;
		}

		*encoder = nullptr;
	}

}