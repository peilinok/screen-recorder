#pragma once

#include "ray_base.h"

namespace ray {
namespace recorder {

class IAudioFrame {
protected:
	virtual ~IAudioFrame() {};

public:

	virtual rt_uid getUID() const = 0;

	virtual const uint8_t *getData() const = 0;

	virtual uint32_t getDataSize() const = 0;
};

class IAudioSource {
protected:
	virtual ~IAudioSource() {};

public:
	virtual void release() = 0;
};

class IAudioSourceManager {
protected:
	virtual ~IAudioSourceManager() {};

public:
	virtual rt_error createAudioSource(AUDIO_SOURCE_TYPE type, IAudioSource **source, rt_uid *uid) = 0;

	virtual void deleteAudioSource(IAudioSource *source) = 0;
};

} // namespace recorder
} // namespace ray