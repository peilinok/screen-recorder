#pragma once


namespace ray {
namespace recorder {

/**
*
*/
class IVideoDeviceCollection {
protected:
	virtual ~IVideoDeviceCollection() {};

	virtual void release() = 0;

	virtual void getCount() = 0;
};

class IVideoDeviceManager {
protected:
	virtual ~IVideoDeviceManager() {};
};

} // namespace recorder
} // namespace ray