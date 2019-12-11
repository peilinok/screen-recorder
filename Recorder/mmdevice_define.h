#pragma once

#ifdef _WIN32

#include <mmdeviceapi.h>
#include <propkeydef.h>//must include before functiondiscoverykeys_devpkey
#include <functiondiscoverykeys_devpkey.h>

#include <wrl/client.h>
#include <devicetopology.h>

class com_initialize {
public:
	com_initialize() {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}
	~com_initialize() {
		CoUninitialize();
	}
};

#endif // _WIN32