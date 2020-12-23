#pragma once

namespace ray {
namespace recorder {

class IAudioEncoderCollection {
protected:
	virtual ~IAudioEncoderCollection() {};
};

class IAudioEncoderManager {
protected:
	virtual ~IAudioEncoderManager() {};
};

} // namespace recorder
} // namespace ray