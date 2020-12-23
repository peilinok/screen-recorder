#pragma once

#include "ray_base.h"

namespace ray {
namespace remuxer {

/**
* Remuxer configuration
*/
typedef struct RemuxerConfiguration {
	int nthread = 4;
}RemuxerConfiguration;

/**
* Remuxer event handler
*/
class IRemuxerEventHandler {
public:
	virtual ~IRemuxerEventHandler() {};
	/**
	* Remux progress callback function
	* @param src source file path
	* @param progress current progress (0-100)
	* @param total total progress value ,will be always 100
	*/
	virtual void onRemuxProgress(const char *src, uint8_t progress, uint8_t total) {}

	/**
	* Remux state
	* @param src     source file path
	* @param succeed true or false
	* @param error   error code
	*/
	virtual void onRemuxState(const char *src, bool succeed, rt_error error) {}
};

/**
* Remuxer
*/
class IRemuxer {
protected:
	virtual ~IRemuxer() {};
public:

	/**
	* Initialize remuxer with specified log file path
	* @param logPath   Specified log file path in utf8
	* @return          0 for success, other for error code
	*/
	virtual rt_error initialize(const RemuxerConfiguration& config) = 0;

	/**
	* Release remuxer
	* This will stop all remuxing tasks and release remuxer
	*/
	virtual void release() = 0;

	/**
	* Set remuxer event handler
	* @param handler
	*/
	virtual void setEventHandler(IRemuxerEventHandler *handler) = 0;

	/**
	* Start to remux file to specified format
	* @param srcFilePath source file path
	* @param dstFilePath dst file path specified format by extension
	* @return 0 for succeed, others for error code
	*/
	virtual rt_error remux(const char* srcFilePath, const char* dstFilePath) = 0;

	/**
	* Stop to remux specified file
	* @param srcFilePath
	*/

	virtual void stop(const char* srcFilePath) = 0;

	/**
	* Stop all remuxing thread
	*/
	virtual void stopAll() = 0;
};

} // namespace remuxer
} // namespace ray

RAY_API ray::remuxer::IRemuxer* RAY_CALL createRemuxer();

