#pragma once

// Auther lzpong
// https://github.com/lzpong/threadpool

#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <stdexcept>

#include "ray_threadpool.h"

#define  THREADPOOL_MAX_NUM 16
//#define  THREADPOOL_AUTO_GROW

namespace ray {
namespace base {

using namespace std;


class ThreadPool
{
	using Task = function<void()>;

	vector<thread> pool_;
	
	queue<Task> tasks_;
	
	mutex mutex_;
	
	condition_variable cv_;
	
	atomic<bool> running_{ true };
	
	atomic<int>  thread_idle_count_{ 0 };

public:
	inline ThreadPool(unsigned short size = 4) { addThread(size); }

	inline ~ThreadPool()
	{
		// set state to not running
		running_ = false;
		
		// weakup all threads
		cv_.notify_all();

		for (thread& thread : pool_) {
			
			// let thread play itself
			//thread.detach();
			
			// wait for thread stopped
			if (thread.joinable())
				thread.join();
		}
	}

	// bind�� .commit(std::bind(&Dog::sayHello, &dog));
	// mem_fn�� .commit(std::mem_fn(&Dog::sayHello), this)
	template<class F, class... Args>
	auto commit(F&& f, Args&&... args) ->future<decltype(f(args...))>
	{
		// threadpool is not running
		if (!running_)
			throw runtime_error("commit on ThreadPool is stopped.");

		using RetType = decltype(f(args...));
		auto task = make_shared<packaged_task<RetType()>>(
			bind(forward<F>(f), forward<Args>(args)...));
		future<RetType> future = task->get_future();
		{
			lock_guard<mutex> lock{ mutex_ };
			tasks_.emplace([task]() {
				(*task)();
			});
		}
#ifdef THREADPOOL_AUTO_GROW
		if (thread_idle_count_ < 1 && pool_.size() < THREADPOOL_MAX_NUM)
			addThread(1);
#endif // !THREADPOOL_AUTO_GROW

		cv_.notify_one();

		return future;
	}

	int idlCount() const { return thread_idle_count_; }
	int thrCount() const { return pool_.size(); }

#ifdef THREADPOOL_AUTO_GROW
private:
#endif // !THREADPOOL_AUTO_GROW

	void addThread(unsigned short size)
	{
		for (; pool_.size() < THREADPOOL_MAX_NUM && size > 0; --size)
		{
			pool_.emplace_back([this] {
				while (running_)
				{
					Task task;
					{
						unique_lock<mutex> lock{ mutex_ };
						cv_.wait(lock, [this] {
							return !running_ || !tasks_.empty();
						});
						if (!running_ && tasks_.empty())
							return;
						task = move(tasks_.front());
						tasks_.pop();
					}
					thread_idle_count_--;
					task();
					thread_idle_count_++;
				}
			});
			thread_idle_count_++;
		}
	}

};

} // namespace base
} // namesapce ray