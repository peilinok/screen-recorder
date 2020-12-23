#include "record_desktop_factory.h"
#include "record_desktop_ffmpeg_gdi.h"
#include "record_desktop_ffmpeg_dshow.h"
#include "record_desktop_gdi.h"
#include "record_desktop_duplication.h"

#include "error_define.h"

int record_desktop_new(am::RECORD_DESKTOP_TYPES type, am::record_desktop ** recorder)
{
	int err = am::AE_NO;
	switch (type)
	{
	case am::DT_DESKTOP_FFMPEG_GDI:
		*recorder = (am::record_desktop*)new am::record_desktop_ffmpeg_gdi();
		break;
	case am::DT_DESKTOP_FFMPEG_DSHOW:
		*recorder = (am::record_desktop*)new am::record_desktop_ffmpeg_dshow();
		break;
	case am::DT_DESKTOP_WIN_GDI:
		*recorder = (am::record_desktop*)new am::record_desktop_gdi();
		break;
	case am::DT_DESKTOP_WIN_DUPLICATION:
		*recorder = (am::record_desktop*)new am::record_desktop_duplication();
		break;
	default:
		err = am::AE_UNSUPPORT;
		break;
	}

	return err;
}

void record_desktop_destroy(am::record_desktop ** recorder)
{
	if (*recorder != nullptr) {
		(*recorder)->stop();

		delete *recorder;
	}

	*recorder = nullptr;
}
