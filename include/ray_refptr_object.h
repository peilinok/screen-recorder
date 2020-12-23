#pragma once

#include <atomic>

#include "ray_refptr.h"

namespace ray {

class RefCounter {
public:
	explicit RefCounter(int count) :count_(count) {}

	RefCounter() = delete;
	RefCounter(RefCounter&) = delete;
	RefCounter& operator=(RefCounter&) = delete;

	void Inc() const { count_.fetch_add(1); }

	RefCountReleaseStatus Dec() const {
		return (count_.fetch_sub(1) == 1) ? RefCountReleaseStatus::kDroppedLastRef
			: RefCountReleaseStatus::kOtherRefsRemained;
	}

	bool HasOneRef() const { return count_.load() == 1; }

private:
	mutable std::atomic<int> count_;
};

/**
* Wrap a class that needs to inherit RefCountInterface in order to be held by ray::ray_refptr
* Usage:
*  ray::ray_refptr<T> ptr = new RefCountedObject<T>(arg1, arg2, ...);
*/

template <class T>
class RefCountedObject : public T {
public:
	RefCountedObject() {}
	RefCountedObject(RefCountedObject&) = delete;
	RefCountedObject& operator=(RefCountedObject&) = delete;

	template <class P0>
	explicit RefCountedObject(P0&& p0) : T(std::forward<P0>(p0)) {}

	template <class P0, class P1, class... Args>
	RefCountedObject(P0&& p0, P1&& p1, Args&&... args)
		: T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<Args>(args)...) {}

	virtual void AddRef() const { counter_.Inc(); }

	virtual RefCountReleaseStatus Release() const {
		const auto status = counter_.Dec();
		if (status == RefCountReleaseStatus::kDroppedLastRef) {
			delete this;
		}
		return status;
	}

	virtual bool HasOneRef() const { return counter_.HasOneRef(); }

protected:
	virtual ~RefCountedObject() {}

	mutable RefCounter counter_{ 0 };
};

} // namespace ray

#define FRIEND_CLASS_REFCOUNTEDOBJECT(T) friend class RefCountedObject<T>
