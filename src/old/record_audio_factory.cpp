#include "record_audio_factory.h"
#include "record_audio_wasapi.h"
#include "record_audio_dshow.h"

#include "error_define.h"


int record_audio_new(am::RECORD_AUDIO_TYPES type,am::record_audio **recorder)
{
	int err = am::AE_NO;

	switch (type)
	{
	case am::AT_AUDIO_WAVE:
		err = am::AE_UNSUPPORT;
		break;
	case am::AT_AUDIO_WAS:
		*recorder = (am::record_audio *)new am::record_audio_wasapi();
		break;
	case am::AT_AUDIO_DSHOW:
		*recorder = (am::record_audio *)new am::record_audio_dshow();
		break;
	case am::AT_AUDIO_FFMPEG:
		err = am::AE_UNSUPPORT;
		break;
	default:
		err = am::AE_UNSUPPORT;
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