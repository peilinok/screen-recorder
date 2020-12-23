#include "ray_audio_device.h"

#include <algorithm>

namespace ray {
namespace devices {

AudioDeviceManager::AudioDeviceManager()
{
}

AudioDeviceManager::~AudioDeviceManager()
{
}

void AudioDeviceManager::registerObserver(IAudioDeviceObserver * observer)
{
	std::lock_guard<std::mutex> lock(observers_mutex_);

	auto itr = std::find_if(observers_.begin(), observers_.end(), 
		[observer](const IAudioDeviceObserver* old) { return observer == old; }
	);

	if (itr == observers_.end())
		observers_.emplace_back(observer);
}

void AudioDeviceManager::unregisterObserver(const IAudioDeviceObserver * observer)
{
	std::lock_guard<std::mutex> lock(observers_mutex_);

	auto itr = std::find_if(observers_.begin(), observers_.end(),
		[observer](const IAudioDeviceObserver* old) { return observer == old; }
	);

	if (itr == observers_.end())
		return;

	observers_.erase(itr);
}

} // devices
} // ray

