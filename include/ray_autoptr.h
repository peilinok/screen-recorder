#pragma once

namespace ray {

template<class T>
class ray_autoptr {
public:
	ray_autoptr(T* p = 0) : ptr_(p) {}
	~ray_autoptr() { if (ptr_) ptr_->release(); }

	ray_autoptr(const ray_autoptr&) = delete;
	ray_autoptr& operator=(const ray_autoptr&) = delete;

	operator bool() const { return ptr_ != (T*)0; }

	T& operator*() const { return *get(); }

	T* operator->() const { return get(); }

	T* get() const { return ptr_; }

	T* release() {
		T* tmp = ptr_;
		ptr_ = 0;
		return tmp;
	}

	void reset(T* ptr = 0) {
		if (ptr != ptr_ && ptr_)
			ptr_->release();
		ptr_ = ptr;
	}

	template<class C1, class C2>
	bool queryInterface(C1* c, C2 iid) {
		T* p = NULL;
		if (c && !c->queryInterface(iid, (void**)&p))
		{
			reset(p);
		}
		return p != NULL;
	}

private:
	T* ptr_;
};

} // namespace ray
