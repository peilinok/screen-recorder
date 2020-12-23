#pragma once

#if defined(WIN32)

#include <windows.h>

#include <mmdeviceapi.h>

// must include before functiondiscoverykeys_devpkey

#include <propkeydef.h>
#include <functiondiscoverykeys_devpkey.h>

#include <wrl/client.h>
#include <devicetopology.h>

#include <propsys.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

#include "base\ray_macro.h"

namespace ray {
namespace utils {

class com_initialize {
public:
	
	com_initialize() {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}

	~com_initialize() {
		CoUninitialize();
	}

private:
	DISALLOW_COPY_AND_ASSIGN(com_initialize);

};

} // namespace ray
} // namespace utils

#endif // _WIN32