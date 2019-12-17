#include "record_desktop_win_gdi.h"

#include "error_define.h"
#include "log_helper.h"

namespace am {

	record_desktop_win_gdi::record_desktop_win_gdi()
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

	record_desktop_win_gdi::~record_desktop_win_gdi()
	{
		stop();
		clean_up();
	}

	int record_desktop_win_gdi::init(const RECORD_DESKTOP_RECT & rect, const int fps)
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
			_time_base = { 1,90000 };
			_pixel_fmt = AV_PIX_FMT_BGRA;


			_inited = true;
		} while (0);

		if (error != AE_NO) {
			al_debug("%s,last error:%ld", err2str(error), GetLastError());
			clean_up();
		}

		return error;
	}

	int record_desktop_win_gdi::start()
	{
		if (_running == true) {
			al_warn("record desktop gdi is already running");
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&record_desktop_win_gdi::record_func, this));

		return AE_NO;
	}

	int record_desktop_win_gdi::pause()
	{
		_paused = true;
		return AE_NO;
	}

	int record_desktop_win_gdi::resume()
	{
		_paused = false;
		return AE_NO;
	}

	int record_desktop_win_gdi::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	void record_desktop_win_gdi::clean_up()
	{
		_inited = false;

		if (_buffer)
			delete[] _buffer;
	}

	bool record_desktop_win_gdi::do_record()
	{
		//int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		//int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		HDC hdc_screen = NULL, hdc_mem = NULL;
		HBITMAP hbm_mem = NULL;

		int error = AE_ERROR;

		do {

			hdc_screen = GetWindowDC(NULL);
			if (!hdc_screen) {
				al_error("get window dc failed:%lld", GetLastError());
				break;
			}

			hdc_mem = CreateCompatibleDC(hdc_screen);
			if (!hdc_mem) {
				al_error("create compatible dc failed:%lld", GetLastError());
				break;
			}

			hbm_mem = CreateCompatibleBitmap(hdc_screen, _width, _height);
			if (!hbm_mem) {
				al_error("create compatible bitmap failed:%lld", GetLastError());
				break;
			}

			SelectObject(hdc_mem, hbm_mem);

			if (!BitBlt(hdc_mem, 0, 0, _width, _height, hdc_screen, _rect.left, _rect.top, SRCCOPY)) {
				al_error("bitblt data failed:%lld", GetLastError());
				break;
			}

			BITMAPINFOHEADER   bi;

			bi.biSize = sizeof(BITMAPINFOHEADER);
			bi.biWidth = _width;
			bi.biHeight = _height * (-1);
			bi.biPlanes = 1;
			bi.biBitCount = 32;
			bi.biCompression = BI_RGB;
			bi.biSizeImage = 0;
			bi.biXPelsPerMeter = 0;
			bi.biYPelsPerMeter = 0;
			bi.biClrUsed = 0;
			bi.biClrImportant = 0;

			//scan colors by line order
			int ret = GetDIBits(hdc_mem, hbm_mem, 0, _height, _buffer, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
			if (ret <= 0 || ret == ERROR_INVALID_PARAMETER) {
				al_error("get dibits failed:%lld", GetLastError());
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

		return (error == AE_NO);
	}

	void record_desktop_win_gdi::record_func()
	{
		AVFrame *frame = av_frame_alloc();

		while (_running)
		{
			
			if (!do_record()) {
				al_error("record desktop by win api failed:%lld", GetLastError());
				break;
			}
			else {
				frame->pts = av_gettime_relative();
				frame->pkt_dts = frame->pts;
				frame->pkt_pts = frame->pts;
				frame->best_effort_timestamp = frame->pts;

				av_image_fill_arrays(frame->data, 
					frame->linesize, 
					_buffer,
					AV_PIX_FMT_BGRA,
					_width, 
					_height, 
					1);

				frame->width = _width;
				frame->height = _height;
				frame->format = AV_PIX_FMT_BGRA;
				frame->pict_type = AV_PICTURE_TYPE_I;
				frame->pkt_size = _width * _height * 4;

				al_debug("%lld", frame->pts);
				if (_on_data) _on_data(frame);

				//av_usleep(60 * 100);
			}
		}

		av_frame_free(&frame);
	}


}