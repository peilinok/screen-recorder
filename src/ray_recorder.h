#pragma once

#include "base\ray_macro.h"
#include "base\ray_singleton.h"

#include "include\ray_base.h"
#include "include\iray_recorder.h"


namespace ray {
namespace recorder {

class Recorder :
	public IRecorder,
	public base::Singleton<Recorder>
{

private:
	Recorder();
	~Recorder();

	FRIEND_SINGLETON(Recorder);
	DISALLOW_COPY_AND_ASSIGN(Recorder);

public:
	rt_error initialize(const RecorderConfiguration& config) override;

	void release() override;

	void setObserver(IRecorderObserver *observer) override;

	rt_error queryInterface(const RECORDER_INTERFACE_IID& iid, void **pp) override;	

	virtual ray_refptr<IAudioDeviceManager> getAudioDeviceManager() override;
private:

	IRecorderObserver *observer_;

	ray_refptr<IAudioDeviceManager> audio_device_mgr_;
};
}
}