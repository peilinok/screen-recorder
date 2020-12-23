#include "system_time.h"

#include <Windows.h>

namespace am {

	static bool got_clockfreq = false;
	static LARGE_INTEGER  clock_freq;

	static uint64_t get_clockfreq() {
		if (!got_clockfreq) {
			QueryPerformanceFrequency(&clock_freq);
			got_clockfreq = true;
		}

		return clock_freq.QuadPart;
	}

	uint64_t system_time::get_time_ns()
	{
		LARGE_INTEGER current_time;
		double time_val;

		QueryPerformanceCounter(&current_time);
		time_val = (double)current_time.QuadPart;
		time_val *= 1000000000.0;
		time_val /= (double)get_clockfreq();

		return (uint64_t)time_val;
	}

}