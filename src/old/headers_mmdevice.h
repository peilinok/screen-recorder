#pragma once

#ifdef _WIN32

#include <windows.h>

#include <mmdeviceapi.h>
#include <propkeydef.h>//must include before functiondiscoverykeys_devpkey
#include <functiondiscoverykeys_devpkey.h>

#include <wrl/client.h>
#include <devicetopology.h>

#include <propsys.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

namespace am {

class com_initialize {
public:
	com_initialize() {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}
	~com_initialize() {
		CoUninitialize();
	}
};
}

#define DEFAULT_AUDIO_INOUTPUT_NAME "Default"
#define DEFAULT_AUDIO_INOUTPUT_ID "Default"

#endif // _WIN32