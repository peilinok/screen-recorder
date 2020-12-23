#ifndef RAY_REMUXER_H
#define RAY_REMUXER_H

#include <map>
#include <atomic>
#include <thread>
#include <memory>

#include "include\iray_remuxer.h"

#include "base\ray_singleton.h"
#include "base\ray_macro.h"

namespace ray {
namespace remuxer {

class RemuxerTask {
public:
	RemuxerTask(const std::string &src,
		const int64_t src_size, const std::string &dst, IRemuxerEventHandler *handler) :
		src_(src), src_size_(src_size), dst_(dst), event_handler_(handler), running_(false) {}

	~RemuxerTask() { stop(); }

	void start();

	bool isRunning() { return running_; }

	void stop();

private:
	void task();

private:
	std::thread thread_;
	std::atomic_bool running_;

	std::string src_;
	std::string dst_;

	int64_t src_size_;

	IRemuxerEventHandler *event_handler_;
};

// should use threadpool later
class Remuxer :
	public remuxer::IRemuxer,
	public remuxer::IRemuxerEventHandler,
	public base::Singleton<Remuxer>
{
private:
	Remuxer() :event_handler_(nullptr) {}
	~Remuxer() { release(); }

	FRIEND_SINGLETON(Remuxer);

public:

	virtual rt_error initialize(const RemuxerConfiguration& config) override;

	virtual void release() override;

	virtual void setEventHandler(IRemuxerEventHandler *handler) override;

	rt_error remux(const char* srcFilePath, const char* dstFilePath) override;

	void stop(const char* srcFilePath) override;

	void stopAll() override;

	virtual void onRemuxProgress(const char *src, uint8_t progress, uint8_t total) override;

	virtual void onRemuxState(const char *src, bool succeed, rt_error error) override;
private:
	std::mutex mutex_;
	std::map<std::string, std::auto_ptr<RemuxerTask>> tasks_;

	IRemuxerEventHandler *event_handler_;
};

} // namespace remuxer

} // namespace ray



#endif // !REMUXER_FFMPEG