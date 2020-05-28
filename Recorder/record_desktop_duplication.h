#ifndef RECORD_DESKTOP_DUPLICATION
#define RECORD_DESKTOP_DUPLICATION

#include "record_desktop.h"

#include <Windows.h>

namespace am {

	class record_desktop_duplication:
		public record_desktop
	{
	public:
		record_desktop_duplication();
		~record_desktop_duplication();

		virtual int init(
			const RECORD_DESKTOP_RECT &rect,
			const int fps);

		virtual int start();
		virtual int pause();
		virtual int resume();
		virtual int stop();

	protected:
		virtual void clean_up();

	private:
		int do_record();

		void record_func();

	};

}

#endif
