#pragma once

#include "ray_base.h"
#include "ray_refptr.h"

namespace ray {
namespace recorder {

class IVideoEncoder :public RefCountInterface {

};

class IVideoEncoderCollection {
protected:
	virtual ~IVideoEncoderCollection() {};
};

class IVideoEncoderManager {
protected:
	virtual ~IVideoEncoderManager() {};
};


} // namespace recorder
} // namespace ray
