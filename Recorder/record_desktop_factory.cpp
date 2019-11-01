#include "record_desktop_factory.h"
#include "record_desktop_gdi.h"

#include "error_define.h"
#include "log_helper.h"

int record_desktop_new(RECORD_DESKTOP_TYPES type, am::record_desktop ** recorder)
{
	int err = AE_NO;
	switch (type)
	{
	case DT_DESKTOP_GDI:
		*recorder = (am::record_desktop*)new am::record_desktop_gdi();
		break;
	default:
		err = AE_UNSUPPORT;
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
