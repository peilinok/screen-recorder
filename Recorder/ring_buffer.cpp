#include "ring_buffer.h"

#include "error_define.h"
#include "log_helper.h"

namespace am {

	template<typename T>
	ring_buffer<T>::ring_buffer(unsigned int size)
	{
		_size = size;
		_head = _tail = 0;

		_buf = new uint8_t[size];
	}

	template<typename T>
	ring_buffer<T>::~ring_buffer()
	{
		if (_buf)
			delete[] _buf;
	}

	template<typename T>
	void ring_buffer<T>::put(const void * data, int len, const T & type)
	{
		std::lock_guard<std::mutex> locker(_lock);

		if (_head + len <= _size) {
			memcpy(_buf + _head, data, len);

			_head += len;
		}
		else if (_head + len > _size) {
			int remain = len - (_size - _head);
			if (len - remain > 0)
				memcpy(_buf + _head, data, len - remain);

			if (remain > 0)
				memcpy(_buf, (unsigned char*)data + len - remain, remain);

			_head = remain;
		}

		struct ring_frame<T> frame;
		frame.len = len;
		frame.type = type;

		_frames.push(frame);
	}

	template<typename T>
	int ring_buffer<T>::get(void * data, int len, T & type)
	{
		std::lock_guard<std::mutex> locker(_lock);

		int retLen = 0;

		if (_frames.size() <= 0) {
			retLen = 0;
			return retLen;
		}

		struct ring_frame<T> frame = _frames.front();
		_frames.pop();

		if (frame.len > len) {
			al_error("ringbuff::get need larger buffer");
			return 0;
		}

		type = frame.type

		retLen = frame.len;

		if (_tail + frame.len <= _size) {

			memcpy(data, _buf + _tail, frame.len);

			_tail += frame.len;
		}
		else {
			int remain = frame.len - (_size - _tail);

			if (frame.len - remain > 0)
				memcpy(data, _buf + _tail, frame.len - remain);

			if (remain > 0)
				memcpy((unsigned char*)data + frame.len - remain, _buf, remain);

			_tail = remain;
		}

		return retLen;
	}
}