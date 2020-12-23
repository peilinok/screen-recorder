#pragma once

#include <list>
#include <mutex>

#include "base\ray_macro.h"

#include "include\iray_audio_device.h"
#include "include\ray_refptr_object.h"


namespace ray {
namespace devices {

using namespace recorder;

class AudioDeviceCollection :public IAudioDeviceCollection {

private:
	AudioDeviceCollection(std::list<AudioDeviceInfo> devices) : devices_(devices) {}

protected:
	virtual ~AudioDeviceCollection() {}

public:
	int getDeviceCount() override { return devices_.size(); }

	virtual AudioDeviceInfo getDeviceInfo(int index) override {
		if (index >= devices_.size())
			return empty_device_;

		auto itr = devices_.begin();

		std::advance(itr, index);

		return *itr;
	};

private:
	AudioDeviceInfo empty_device_;
	std::list<AudioDeviceInfo> devices_;

private:
	friend class AudioDeviceManager;

	DISALLOW_COPY_AND_ASSIGN(AudioDeviceCollection);
	FRIEND_CLASS_REFCOUNTEDOBJECT(AudioDeviceCollection);
};

class AudioDeviceManager : public IAudioDeviceManager {

private:
	AudioDeviceManager();

protected:
	virtual ~AudioDeviceManager();

public:

	void registerObserver(IAudioDeviceObserver* observer) override;

	void unregisterObserver(const IAudioDeviceObserver* observer) override;

	int setMicrophone(const char id[kAdmDeviceIdSize]) override;

	int setMicrophoneVolume(const unsigned int volume) override;

	int getMicrophoneVolume(unsigned int& volume) override;

	ray_refptr<IAudioDeviceCollection> getMicrophoneCollection() override;

	int setSpeaker(const char id[kAdmDeviceIdSize]) override;

	int setSpeakerVolume(const unsigned int volume) override;

	int getSpeakerVolume(unsigned int& volume) override;

	ray_refptr<IAudioDeviceCollection> getSpeakerCollection() override;

private:
#if defined(WIN32)

	int getAudioDevices(bool capture, std::list<AudioDeviceInfo>& devices);

#endif

private:
	std::mutex observers_mutex_;

	std::list<IAudioDeviceObserver*> observers_;

private:
	DISALLOW_COPY_AND_ASSIGN(AudioDeviceManager);
	FRIEND_CLASS_REFCOUNTEDOBJECT(AudioDeviceManager);
};

} // devices
} // ray