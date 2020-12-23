#ifndef SYSTEM_TIME
#define SYSTEM_TIME

#include <stdint.h>

namespace am {

	class system_time
	{
	private:
		system_time() {};
		~system_time() {};
	public:
		static uint64_t get_time_ns();
	};

}

#endif // !SYSTEM_TIME