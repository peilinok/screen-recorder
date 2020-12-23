#include "record_desktop_gdi.h"

#include "error_define.h"

#include "utils\ray_log.h"

namespace am {

	record_desktop_gdi::record_desktop_gdi()
	{
		_data_type = RECORD_DESKTOP_DATA_TYPES::AT_DESKTOP_BGRA;
		_buffer = NULL;
		_buffer_size = 0;

		_draw_cursor = true;

		_hdc = NULL;
		_bmp = NULL;
		_bmp_old = NULL;
		_ci = { 0 };
	}

	record_desktop_gdi::~record_desktop_gdi()
	{
		stop();
		clean_up();
	}

	int record_desktop_gdi::init(const RECORD_DESKTOP_RECT & rect, const int fps)
	{
		int error = AE_NO;
		if (_inited == true) {
			return error;
		}

		_fps = fps;
		_rect = rect;


		do {
			_width = rect.right - rect.left;
			_height = rect.bottom - rect.top;
			_buffer_size = (_width * 32 + 31) / 32 * _height * 4;
			_buffer = new uint8_t[_buffer_size];

			_start_time = av_gettime_relative();
			_time_base = { 1,AV_TIME_BASE };
			_pixel_fmt = AV_PIX_FMT_BGRA;


			_inited = true;
		} while (0);

		if (error != AE_NO) {
			LOG(ERROR) << "record desktop gdi init failed: " << (err2str(error)) << " ,last error: " << GetLastError();
			clean_up();
		}

		return error;
	}

	int record_desktop_gdi::start()
	{
		if (_running == true) {
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&record_desktop_gdi::record_func, this));

		return AE_NO;
	}

	int record_desktop_gdi::pause()
	{
		_paused = true;
		return AE_NO;
	}

	int record_desktop_gdi::resume()
	{
		_paused = false;
		return AE_NO;
	}

	int record_desktop_gdi::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	void record_desktop_gdi::clean_up()
	{
		_inited = false;

		if (_buffer)
			delete[] _buffer;
	}

	void record_desktop_gdi::draw_cursor(HDC hdc)
	{
		if (!(_ci.flags & CURSOR_SHOWING))
			return;

		//is cursor in the tartet zone
		if (_ci.ptScreenPos.x < _rect.left ||
			_ci.ptScreenPos.x > _rect.right ||
			_ci.ptScreenPos.y < _rect.top ||
			_ci.ptScreenPos.y > _rect.bottom
			)
			return;

		HICON icon;
		ICONINFO ii;

		icon = CopyIcon(_ci.hCursor);
		if (!icon)
			return;

		int dstx = 0, dsty = 0;
		dstx = abs(_ci.ptScreenPos.x - _rect.left);
		dsty = abs(_ci.ptScreenPos.y - _rect.top);

		if (GetIconInfo(icon, &ii)) {
			POINT pos;
			DrawIconEx(hdc, dstx, dsty, icon, 0, 0, 0, NULL, DI_NORMAL);

			DeleteObject(ii.hbmColor);
			DeleteObject(ii.hbmMask);
		}

		DestroyIcon(icon);
	}

	int record_desktop_gdi::do_record()
	{
		//int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		//int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		HDC hdc_screen = NULL, hdc_mem = NULL;
		HBITMAP hbm_mem = NULL;

		int error = AE_ERROR;

		do {

			hdc_screen = GetWindowDC(NULL);
			if (!hdc_screen) {
				LOG(ERROR) << "get window dc failed: " << GetLastError();
				error = AE_GDI_GET_DC_FAILED;
				break;
			}

			hdc_mem = CreateCompatibleDC(hdc_screen);
			if (!hdc_mem) {
				LOG(ERROR) << "create compatible dc failed: " << GetLastError();
				error = AE_GDI_CREATE_DC_FAILED;
				break;
			}

			hbm_mem = CreateCompatibleBitmap(hdc_screen, _width, _height);
			if (!hbm_mem) {
				LOG(ERROR) << "create compatible bitmap failed: " << GetLastError();
				error = AE_GDI_CREATE_BMP_FAILED;
				break;
			}

			SelectObject(hdc_mem, hbm_mem);

			//must have CAPTUREBLT falg,otherwise some layered window can not be captured
			if (!BitBlt(hdc_mem, 0, 0, _width, _height, hdc_screen, _rect.left, _rect.top, SRCCOPY | CAPTUREBLT)) {
				LOG(ERROR) << "bitblt data failed: " << GetLastError();
				//error = AE_GDI_BITBLT_FAILED;
				//administrator UAC will trigger invalid handle error
				break;
			}

			memset(&_ci, 0, sizeof(CURSORINFO));
			_ci.cbSize = sizeof(CURSORINFO);
			if (GetCursorInfo(&_ci)) {
				draw_cursor(hdc_mem);
			}

			BITMAPINFOHEADER   bi;

			bi.biSize = sizeof(BITMAPINFOHEADER);
			bi.biWidth = _width;
			bi.biHeight = _height * (-1);
			bi.biPlanes = 1;
			bi.biBitCount = 32;//should get from system color bits
			bi.biCompression = BI_RGB;
			bi.biSizeImage = 0;
			bi.biXPelsPerMeter = 0;
			bi.biYPelsPerMeter = 0;
			bi.biClrUsed = 0;
			bi.biClrImportant = 0;

			//scan colors by line order
			int ret = GetDIBits(hdc_mem, hbm_mem, 0, _height, _buffer, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
			if (ret <= 0 || ret == ERROR_INVALID_PARAMETER) {
				LOG(ERROR) << "get dibits failed: " << GetLastError();
				error = AE_GDI_GET_DIBITS_FAILED;
				break;
			}

#if 0
			//save bmp to test
			BITMAPFILEHEADER bf;
			bf.bfType = 0x4d42;
			bf.bfReserved1 = 0;
			bf.bfReserved2 = 0;
			bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
			bf.bfSize = bf.bfOffBits + _width * _height * 4;

			FILE *fp = fopen("..\\..\\save.bmp", "wb+");

			fwrite(&bf, 1, sizeof(bf), fp);
			fwrite(&bi, 1, sizeof(bi), fp);
			fwrite(_buffer, 1, _buffer_size, fp);

			fflush(fp);
			fclose(fp);
#endif
			error = AE_NO;
		} while (0);

		if(hbm_mem)
			DeleteObject(hbm_mem);

		if(hdc_mem)
			DeleteObject(hdc_mem);

		if(hdc_screen)
			ReleaseDC(NULL, hdc_screen);

		return  AE_NO;
	}

	void record_desktop_gdi::do_sleep(int64_t dur, int64_t pre, int64_t now)
	{
		int64_t delay = now - pre;
		dur = delay > dur ? max(0, dur - (delay - dur)) : (dur + dur - delay);

		//al_debug("%lld", delay);

		if(dur)
			av_usleep(dur);
	}

	void record_desktop_gdi::record_func()
	{
		AVFrame *frame = av_frame_alloc();

		int64_t pre_pts = 0;
		int64_t dur = AV_TIME_BASE / _fps;

		int ret = AE_NO;
		while (_running)
		{
			ret = do_record();
			if (ret != AE_NO) {
				if (_on_error) _on_error(ret);
				break;
			}

			frame->pts = av_gettime_relative();
			frame->pkt_dts = frame->pts;

			frame->width = _width;
			frame->height = _height;
			frame->format = AV_PIX_FMT_BGRA;
			frame->pict_type = AV_PICTURE_TYPE_I;
			frame->pkt_size = _width * _height * 4;

			av_image_fill_arrays(frame->data, 
				frame->linesize, 
				_buffer,
				AV_PIX_FMT_BGRA,
				_width,
				_height,
				1
			);

			if (_on_data) _on_data(frame);

			do_sleep(dur, pre_pts, frame->pts);

			pre_pts = frame->pts;
		}

		av_frame_free(&frame);
	}


}