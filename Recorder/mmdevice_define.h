#pragma once

#ifdef _WIN32


#include <MMDeviceAPI.h>


class com_initialize {
public:
	com_initialize() {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}
	~com_initialize() {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}
};

#endif // _WIN32