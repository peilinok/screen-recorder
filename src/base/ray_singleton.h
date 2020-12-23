#pragma once

#include <memory>
#include <mutex>

namespace ray {
namespace base {

template<typename T>
class Singleton
{
public:
	static T* getInstance() {
		static std::mutex mutex_;
		if (instance_.get() == nullptr) {
			std::lock_guard<std::mutex> lock(mutex_);
			if (instance_.get() == nullptr)
				instance_.reset(new T());
		}
		return instance_.get();
	}

protected:
	Singleton() {}
	virtual ~Singleton() {}

private:
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

	static std::auto_ptr<T> instance_;
};

template <typename T>
std::auto_ptr<T> Singleton<T>::instance_;

} // namespace utils
} // namespace ray

  // coz Singleton and auto_ptr need to call 
  // construct and deconstruct of Recorder
#define FRIEND_SINGLETON(TypeName) \
  friend ray::base::Singleton<TypeName>; \
  friend class std::auto_ptr<TypeName>