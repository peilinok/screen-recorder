#include "record_audio_factory.h"
#include "record_audio_wasapi.h"

#include "error_define.h"
#include "log_helper.h"


int record_audio_new(RECORD_AUDIO_TYPES type,am::record_audio **recorder)
{
	int err = AE_NO;

	switch (type)
	{
	case AT_AUDIO_WAVE:
		err = AE_UNSUPPORT;
		break;
	case AT_AUDIO_WAS:
		*recorder = (am::record_audio *)new am::record_audio_wasapi();
		break;
	case AT_AUDIO_DSHOW:
		err = AE_UNSUPPORT;
		break;
	case AT_AUDIO_FFMPEG:
		err = AE_UNSUPPORT;
		break;
	default:
		err = AE_UNSUPPORT;
		break;
	}

	return err;
}

void record_audio_destroy(am::record_audio ** recorder)
{
	if (*recorder != nullptr) {
		(*recorder)->stop();
		delete *recorder;
	}

	*recorder = nullptr;
}