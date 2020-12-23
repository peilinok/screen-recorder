#pragma once

#include <memory>

namespace ray {

enum class RefCountReleaseStatus { kDroppedLastRef, kOtherRefsRemained };

class RefCountInterface {
protected:
	virtual ~RefCountInterface() = default;

public:
	virtual void AddRef() const = 0;

	virtual RefCountReleaseStatus Release() const = 0;

	virtual bool HasOneRef() const = 0;
};

template<class T>
class ray_refptr {
public:
	ray_refptr() : ptr_(nullptr) {}

	ray_refptr(T* p) : ptr_(p) { if (ptr_) ptr_->AddRef(); }

	template<typename U>
	ray_refptr(U* p) : ptr_(p) { if (ptr_) ptr_->AddRef(); }

	ray_refptr(const ray_refptr<T>& r) : ray_refptr(r.get()) {}

	template <typename U>
	ray_refptr(const ray_refptr<U>& r) : ray_refptr(r.get()) {}

	ray_refptr(ray_refptr<T>&& r) : ptr_(r.move()) {}

	template <typename U>
	ray_refptr(ray_refptr<U>&& r) : ptr_(r.move()) {}

	~ray_refptr() { reset(); }

	T* get() const { return ptr_; }

	// Allow ray_refptr<C> to be used in boolean expression
	// and comparison operations.
	operator bool() const { return (ptr_ != nullptr); }

	T* operator->() const { return ptr_; }

	// Returns the (possibly null) raw pointer, and makes the ray_refptr hold a
	// null pointer, all without touching the reference count of the underlying
	// pointed-to object. The object is still reference counted, and the caller of
	// move() is now the proud owner of one reference, so it is responsible for
	// calling Release() once on the object when no longer using it.
	T* move() {
		T* retVal = ptr_;
		ptr_ = nullptr;
		return retVal;
	}

	ray_refptr<T>& operator=(T* p) {
		if (ptr_ == p) return *this;

		if (p) p->AddRef();
		if (ptr_) ptr_->Release();
		ptr_ = p;
		return *this;
	}

	ray_refptr<T>& operator=(const ray_refptr<T>& r) {
		return *this = r.ptr_;
	}

	ray_refptr<T>& operator=(ray_refptr<T>&& r) {
		ray_refptr<T>(std::move(r)).swap(*this);
		return *this;
	}

	template <typename U>
	ray_refptr<T>& operator=(ray_refptr<U>&& r) {
		ray_refptr<T>(std::move(r)).swap(*this);
		return *this;
	}

	// For working with std::find()
	bool operator==(const ray_refptr<T>& r) const { return ptr_ == r.ptr_; }

	// For working with std::set
	bool operator<(const ray_refptr<T>& r) const { return ptr_ < r.ptr_; }

	void swap(T** pp) {
		T* p = ptr_;
		ptr_ = *pp;
		*pp = p;
	}

	void swap(ray_refptr<T>& r) { swap(&r.ptr_); }

	void reset() {
		if (ptr_) {
			ptr_->Release();
			ptr_ = nullptr;
		}
	}

protected:
	T* ptr_;
};

} //namespace ray
