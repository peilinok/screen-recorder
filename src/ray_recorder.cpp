#include "ray_recorder.h"
#include "ray_remuxer.h"

#include "devices\ray_audio_device.h"

#include "utils\ray_string.h"
#include "utils\ray_log.h"

namespace ray {
namespace recorder {

using namespace base;

Recorder::Recorder():
	observer_(nullptr),
	audio_device_mgr_(nullptr)
{

}

Recorder::~Recorder()
{
	release();
}

rt_error Recorder::initialize(const RecorderConfiguration& config) {

	utils::InitLogImpl(config.logPath ? utils::strings::utf8_unicode(config.logPath).c_str() : nullptr);

	return ERR_NO;
}

void Recorder::release() {

}

void Recorder::setObserver(IRecorderObserver *observer) {
	observer_ = observer;
}

rt_error Recorder::queryInterface(const RECORDER_INTERFACE_IID& iid, void **pp) {
	rt_error ret = ERR_NO;

	do {
		if (!pp) {
			ret = ERR_INVALID_CONTEXT;
			break;
		}

		*pp = nullptr;

		switch (iid)
		{
			case RECORDER_IID_VIDEO_SOURCE_MGR:
				*pp = static_cast<void*>(remuxer::Remuxer::getInstance());
			break;
			case RECORDER_IID_AUDIO_DEVICE_MGR:
				break;
		default:
			ret = ERR_UNSUPPORT;
			break;
		}

	} while (0);

	return ret;
}

ray_refptr<IAudioDeviceManager> Recorder::getAudioDeviceManager()
{
	using namespace devices;

	// multiple override
	if (!audio_device_mgr_)
		audio_device_mgr_ = new RefCountedObject<AudioDeviceManager>();


	return audio_device_mgr_;
}

} // recorder
} // ray