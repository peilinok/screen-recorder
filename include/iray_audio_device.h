#pragma once

#include <string>

#include "ray_refptr.h"

namespace ray {
namespace recorder {

static const int kAdmDeviceNameSize = 128;
static const int kAdmDeviceIdSize = 128;

/**
* AudioDeviceInfo struct.
*/
struct AudioDeviceInfo {

	/**
	* Name of audio device.
	*/
	char name[kAdmDeviceNameSize] = { 0 };
	
	/**
	* ID of audio device.
	*/
	char id[kAdmDeviceIdSize] = { 0 };
	
	/**
	* Whether the device is default selected.
	*/
	bool isDefaultSelecteed{ false };

	/**
	* Whether the device is current selected to capture or play audio by system.
	*/
	bool isCurrentSelected{ false };
	
	/**
	* Whether the device is a microphone or speaker.
	*/
	bool isMicrophone{ false };
};

/**
* Collection of audio devices.
*/
class IAudioDeviceCollection : public RefCountInterface {
protected:
	virtual ~IAudioDeviceCollection() = default;

public:

	/**
	* Get the count of audio devices in the collection.
	* @return Count of audio devices.
	*/
	virtual int getDeviceCount() = 0;

	/**
	* Get information of specified audio device by index.
	* @param index Index of audio device.
	* @return AudioDeviceInfo.
	*/
	virtual AudioDeviceInfo getDeviceInfo(int index) = 0;
};

/**
* Observer of audio devices.
*/
class IAudioDeviceObserver {
public:
	virtual ~IAudioDeviceObserver() = default;

	/**
	* Occurs when the audio device state changes, such as plugin or unplugin
	* You should refresh your device list when this occured by call 
	* getMicrophoneCollection() or getSpeakerCollection().
	*/
	virtual void onDeviceStateChanged() = 0;
};

/**
* Manager of audio devices.
*/
class IAudioDeviceManager : public RefCountInterface {
protected:
	virtual ~IAudioDeviceManager() = default;
public:
	/**
	* Registers an IAudioDeviceObserver.
	* You must call unregisterObserver() when you do not need any more.
	* @param observer IAudioDeviceObserver.
	*/
	virtual void registerObserver(IAudioDeviceObserver* observer) = 0;

	/**
	* Ungisters an IAudioDeviceObserver.
	* @param observer IAudioDeviceObserver.
	*/
	virtual void unregisterObserver(const IAudioDeviceObserver* observer) = 0;

	/**
	* Select microphone by id
	* @param id Device id.
	* @return int 0 for success, others for error code.
	*/
	virtual int setMicrophone(const char id[kAdmDeviceIdSize]) = 0;
	
	/**
	* Set volume of current selected microphone.
	* @param volume [0, 255].
	* @return int 0 for success, others for error code.
	*/
	virtual int setMicrophoneVolume(const unsigned int volume) = 0;

	/**
	* Get current microphone of system.
	* @param volume Reference of volume [0, 255].
	* @return int 0 for success, others for error code.
	*/
	virtual int getMicrophoneVolume(unsigned int& volume) = 0;

	/**
	* Get collection of microphones.
	* @return ray_refptr<IAudioDeviceCollection>
	*/
	virtual ray_refptr<IAudioDeviceCollection> getMicrophoneCollection() = 0;

	/**
	* Select speaker by id
	* @param id Device id.
	* @return int 0 for success, others for error code.
	*/
	virtual int setSpeaker(const char id[kAdmDeviceIdSize]) = 0;

	/**
	* Set volume of current selected speaker.
	* @param volume [0, 255].
	* @return int 0 for success, others for error code.
	*/
	virtual int setSpeakerVolume(const unsigned int volume) = 0;

	/**
	* Get current speaker of system.
	* @param volume Reference of volume [0, 255].
	* @return int 0 for success, others for error code.
	*/
	virtual int getSpeakerVolume(unsigned int& volume) = 0;

	/**
	* Get collection of speakers.
	* @return ray_refptr<IAudioDeviceCollection>
	*/
	virtual ray_refptr<IAudioDeviceCollection> getSpeakerCollection() = 0;
};

} // namespace recorder
} // namespace ray