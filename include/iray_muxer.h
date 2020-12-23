#pragma once

#include "ray_base.h"

namespace ray {
namespace recorder {

class IMuxer {
protected:
	virtual ~IMuxer() {};

public:
	virtual bool isMuxing() = 0;

	virtual rt_error start(const char outputFileName[RECORDER_MAX_PATH_LEN]) = 0;

	virtual void stop() = 0;

	virtual rt_error pause() = 0;

	virtual rt_error resume() = 0;
};

} // namespace recorder
} // namespace ray