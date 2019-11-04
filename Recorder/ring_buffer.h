#ifndef RING_BUFFER
#define RING_BUFFER

#include <stdio.h>
#include <stdlib.h>

#include <queue>
#include <mutex>

namespace am {

	typedef struct _ring_frame {
		uint8_t type;
		int len;
	}ring_frame;

	class ring_buffer
	{
	public:
		ring_buffer(unsigned int size = 1920 * 1080 * 4 * 10);
		~ring_buffer();

		void put(const void *data, int len, uint8_t type = -1);
		int get(void *data, int len, uint8_t *type = NULL);

	private:
		std::queue<ring_frame> _frames;
		unsigned int _size, _head, _tail;

		uint8_t *_buf;

		std::mutex _lock;
	};

}
#endif