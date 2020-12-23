#ifndef RECORD_DESKTOP_GDI
#define RECORD_DESKTOP_GDI

#include "record_desktop.h"

#include <Windows.h>

namespace am {

	class record_desktop_gdi :
		public record_desktop
	{
	public:
		record_desktop_gdi();
		~record_desktop_gdi();

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
		void draw_cursor(HDC hdc);

		int do_record();

		void do_sleep(int64_t dur, int64_t pre, int64_t now);

		void record_func();

		uint8_t *_buffer;
		uint32_t _buffer_size;
		uint32_t _width, _height;

		std::atomic_bool _draw_cursor;

		HDC _hdc;
		HBITMAP _bmp, _bmp_old;
		CURSORINFO _ci;
	};

}

#endif