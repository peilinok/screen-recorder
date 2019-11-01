#include "stdafx.h"
#include "record_desktop.h"

am::record_desktop::record_desktop()
{
	_running = false;
	_inited = false;

	_on_data = nullptr;
	_on_error = nullptr;

	_device_name = "";
	_data_type = RECORD_DESKTOP_DATA_TYPES::AT_DESKTOP_BGRA;

	//memset(&_record_rect, 0, sizeof(_record_rect));
}

am::record_desktop::~record_desktop()
{
}
