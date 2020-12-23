#pragma once

#include "ray_base.h"
#include "ray_refptr.h"

namespace ray {
namespace recorder {

class IVideoFrame {
public:

	enum VIDEO_FRAME_TYPE {
		VIDEO_FRAME_BGRA,
		VIDEO_FRAME_YUV420,
		VIDEO_FRAME_YUV444
	};

	virtual rt_uid getUID() const = 0;

	virtual const uint8_t *getData() const = 0;

	virtual uint32_t getDataSize() const = 0;

	virtual const CRect& getSize() const = 0;

protected:
	virtual ~IVideoFrame() {};
};

class IVideoSource :public RefCountInterface {
public:

protected:
	~IVideoSource() {};
};

class IVideoSourceManager {
public:
	virtual rt_error createVideoSource(VIDEO_SOURCE_TYPE type, IVideoSource **source, rt_uid *uid) = 0;

	virtual void deleteVideoSource(IVideoSource *source) = 0;

protected:
	virtual ~IVideoSourceManager() {};
};

} // namespace recorder
} // namespace ray

