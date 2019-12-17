#ifndef RECORD_DESKTOP_WIN_GDI
#define RECORD_DESKTOP_WIN_GDI

#include "record_desktop.h"

#include <Windows.h>

namespace am {

	class record_desktop_win_gdi :
		public record_desktop
	{
	public:
		record_desktop_win_gdi();
		~record_desktop_win_gdi();

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
		bool do_record();
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