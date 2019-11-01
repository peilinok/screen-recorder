#include "record_devices.h"

#include "error_define.h"
#include "log_helper.h"

#include <portaudio.h>

namespace am {

	record_devices::record_devices()
	{
	}


	record_devices::~record_devices()
	{
	}

	int record_devices::get_input_devices(std::list<std::string> devices)
	{
		return 0;
	}

}