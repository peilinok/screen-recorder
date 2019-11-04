#include "ring_buffer.h"

#include "error_define.h"
#include "log_helper.h"

namespace am {

	ring_buffer::ring_buffer(unsigned int size)
	{
		_size = size;
		_head = _tail = 0;

		_buf = new uint8_t[size];
	}

	ring_buffer::~ring_buffer()
	{
		if (_buf)
			delete[] _buf;
	}

	void ring_buffer::put(const void * data, int len, uint8_t type)
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

		ring_frame frame;
		frame.len = len;
		frame.type = type;

		_frames.push(frame);
	}

	int ring_buffer::get(void * data, int len, uint8_t * type)
	{
		std::lock_guard<std::mutex> locker(_lock);

		int retLen = 0;

		if (_frames.size() <= 0) {
			retLen = 0;
			return retLen;
		}

		ring_frame frame = _frames.front();
		_frames.pop();

		if (frame.len > len) {
			al_error("ringbuff::get need larger buffer");
			return 0;
		}


		if (type != NULL)
			*type = frame.type;

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