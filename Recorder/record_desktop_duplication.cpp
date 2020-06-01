#include "record_desktop_duplication.h"

#include "system_lib.h"
#include "d3d_pixelshader.h"
#include "d3d_vertexshader.h"

#include "error_define.h"
#include "log_helper.h"


namespace am {

	record_desktop_duplication::record_desktop_duplication()
	{
		_data_type = RECORD_DESKTOP_DATA_TYPES::AT_DESKTOP_BGRA;
		_buffer = NULL;
		_buffer_size = 0;

		_d3d11 = nullptr;
		_dxgi = nullptr;
		
		_d3d_device = nullptr;
		_d3d_ctx = nullptr;
		_d3d_vshader = nullptr;
		_d3d_inlayout = nullptr;
		_d3d_samplerlinear = nullptr;

		_duplication = nullptr;
		_image = nullptr;
		_output_des = { 0 };
		_output_index = 0;

		ZeroMemory(&_cursor_info, sizeof(_cursor_info));
	}


	record_desktop_duplication::~record_desktop_duplication()
	{
		stop();
		clean_up();
	}

	int record_desktop_duplication::init(const RECORD_DESKTOP_RECT & rect, const int fps)
	{
		int error = AE_NO;
		if (_inited == true) {
			return error;
		}

		_fps = fps;
		_rect = rect;

		do {
			_d3d11 = load_system_library("d3d11.dll");
			_dxgi = load_system_library("dxgi.dll");

			if (!_d3d11 || !_dxgi) {
				error = AE_D3D_LOAD_FAILED;
				break;
			}

			error = init_d3d11();
			if (error != AE_NO)
				break;

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
			al_debug("%s,last error:%lu", err2str(error), GetLastError());
			clean_up();
		}

		return error;
	}

	int record_desktop_duplication::start()
	{
		if (_running == true) {
			al_warn("record desktop duplication is already running");
			return AE_NO;
		}

		if (_inited == false) {
			return AE_NEED_INIT;
		}

		_running = true;
		_thread = std::thread(std::bind(&record_desktop_duplication::record_func, this));

		return AE_NO;
	}

	int record_desktop_duplication::pause()
	{
		_paused = true;
		return AE_NO;
	}

	int record_desktop_duplication::resume()
	{
		_paused = false;
		return AE_NO;
	}

	int record_desktop_duplication::stop()
	{
		_running = false;
		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	void record_desktop_duplication::clean_up()
	{
		_inited = false;

		if (_buffer)
			delete[] _buffer;

		if (_cursor_info.buff) {
			delete[] _cursor_info.buff;
			_cursor_info.buff = nullptr;
		}

		ZeroMemory(&_cursor_info, sizeof(_cursor_info));

		//clean up duplication interfaces
		clean_duplication();

		//clean up d3d11 interfaces
		clean_d3d11();

		//finally free d3d11 & dxgi library
		if (_d3d11) free_system_library(_d3d11);
		if (_dxgi) free_system_library(_dxgi);
	}

	int record_desktop_duplication::init_d3d11()
	{
		int error = AE_NO;

		do {
			PFN_D3D11_CREATE_DEVICE create_device;

			create_device = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(
				_d3d11, "D3D11CreateDevice");
			if (!create_device) {
				error = AE_D3D_GET_PROC_FAILED;
				break;
			}

			HRESULT hr = S_OK;

			// Driver types supported
			D3D_DRIVER_TYPE driver_types[] =
			{
				D3D_DRIVER_TYPE_HARDWARE,
				D3D_DRIVER_TYPE_WARP,
				D3D_DRIVER_TYPE_REFERENCE,
			};
			UINT n_driver_types = ARRAYSIZE(driver_types);

			// Feature levels supported
			D3D_FEATURE_LEVEL feature_levels[] =
			{
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_1
			};
			UINT n_feature_levels = ARRAYSIZE(feature_levels);

			D3D_FEATURE_LEVEL feature_level;

			// Create device
			for (UINT driver_index = 0; driver_index < n_driver_types; ++driver_index)
			{
				hr = create_device(nullptr, driver_types[driver_index], nullptr, 0, feature_levels, n_feature_levels,
					D3D11_SDK_VERSION, &_d3d_device, &feature_level, &_d3d_ctx);
				if (SUCCEEDED(hr))
				{
					// Device creation success, no need to loop anymore
					break;
				}
			}
			if (FAILED(hr))
			{
				error = AE_D3D_CREATE_DEVICE_FAILED;
				break;
			}

			// VERTEX shader
			UINT Size = ARRAYSIZE(g_VS);
			hr = _d3d_device->CreateVertexShader(g_VS, Size, nullptr, &_d3d_vshader);
			if (FAILED(hr))
			{
				error = AE_D3D_CREATE_VERTEX_SHADER_FAILED;
				break;
			}

			// Input layout
			D3D11_INPUT_ELEMENT_DESC layouts[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			UINT n_layouts = ARRAYSIZE(layouts);
			hr = _d3d_device->CreateInputLayout(layouts, n_layouts, g_VS, Size, &_d3d_inlayout);
			if (FAILED(hr))
			{
				error = AE_D3D_CREATE_INLAYOUT_FAILED;
				break;
			}
			_d3d_ctx->IASetInputLayout(_d3d_inlayout);

			// Pixel shader
			Size = ARRAYSIZE(g_PS);
			hr = _d3d_device->CreatePixelShader(g_PS, Size, nullptr, &_d3d_pshader);
			if (FAILED(hr))
			{
				error = AE_D3D_CREATE_PIXEL_SHADER_FAILED;
				break;
			}

			// Set up sampler
			D3D11_SAMPLER_DESC sampler_desc;
			RtlZeroMemory(&sampler_desc, sizeof(sampler_desc));
			sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			sampler_desc.MinLOD = 0;
			sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = _d3d_device->CreateSamplerState(&sampler_desc, &_d3d_samplerlinear);
			if (FAILED(hr))
			{
				error = AE_D3D_CREATE_SAMPLERSTATE_FAILED;
				break;
			}
		} while (0);

		return error;
	}

	void record_desktop_duplication::clean_d3d11()
	{
		if (_d3d_device) _d3d_device->Release();
		if (_d3d_ctx) _d3d_ctx->Release();
		if (_d3d_vshader) _d3d_vshader->Release();
		if (_d3d_pshader) _d3d_pshader->Release();
		if (_d3d_inlayout) _d3d_inlayout->Release();
		if (_d3d_samplerlinear) _d3d_samplerlinear->Release();
	}

	int record_desktop_duplication::init_duplication()
	{
		int error = AE_NO;
		do {
			// Get DXGI device
			IDXGIDevice* dxgi_device = nullptr;
			HRESULT hr = _d3d_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));
			if (FAILED(hr))
			{
				error = AE_DUP_QI_FAILED;
				break;
			}

			// Get DXGI adapter
			IDXGIAdapter* dxgi_adapter = nullptr;
			hr = dxgi_device->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgi_adapter));
			dxgi_device->Release();
			dxgi_device = nullptr;
			if (FAILED(hr))
			{
				error = AE_DUP_GET_PARENT_FAILED;
				break;
			}

			// Get output
			IDXGIOutput* dxgi_output = nullptr;
			hr = dxgi_adapter->EnumOutputs(_output_index, &dxgi_output);
			dxgi_adapter->Release();
			dxgi_adapter = nullptr;
			if (FAILED(hr))
			{
				error = AE_DUP_ENUM_OUTPUT_FAILED;
				break;
			}

			dxgi_output->GetDesc(&_output_des);

			// QI for Output 1
			IDXGIOutput1* dxgi_output1 = nullptr;
			hr = dxgi_output->QueryInterface(__uuidof(dxgi_output1), reinterpret_cast<void**>(&dxgi_output1));
			dxgi_output->Release();
			dxgi_output = nullptr;
			if (FAILED(hr))
			{
				error = AE_DUP_QI_FAILED;
				break;
			}

			// Create desktop duplication
			hr = dxgi_output1->DuplicateOutput(_d3d_device, &_duplication);
			dxgi_output1->Release();
			dxgi_output1 = nullptr;
			if (FAILED(hr))
			{
				error = AE_DUP_DUPLICATE_FAILED;
				if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
				{
					error = AE_DUP_DUPLICATE_MAX_FAILED;
				}
				break;
			}
		} while (0);

		return error;
	}

	int record_desktop_duplication::free_duplicated_frame()
	{
		HRESULT hr = _duplication->ReleaseFrame();
		if (FAILED(hr))
		{
			return AE_DUP_RELEASE_FRAME_FAILED;
		}

		if (_image)
		{
			_image->Release();
			_image = nullptr;
		}

		return AE_DUP_RELEASE_FRAME_FAILED;
	}

	void record_desktop_duplication::clean_duplication()
	{
		if (_duplication) _duplication->Release();
		if (_image) _image->Release();
	}

	bool record_desktop_duplication::attatch_desktop()
	{
		HDESK desktop = nullptr;
		desktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
		if (!desktop)
		{
			// We do not have access to the desktop so request a retry
			return false;
		}

		// Attach desktop to this thread
		bool battached = SetThreadDesktop(desktop) != 0;
		CloseDesktop(desktop);

		if (!battached)
		{
			// We do not have access to the desktop so request a retry
			return false;
		}

		return true;
	}

	int record_desktop_duplication::get_desktop_image(DXGI_OUTDUPL_FRAME_INFO *frame_info)
	{
		IDXGIResource* dxgi_res = nullptr;

		// get new frame
		HRESULT hr = _duplication->AcquireNextFrame(500, frame_info, &dxgi_res);

		// timeout will return when desktop has no chane
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) return AE_TIMEOUT;

		if (FAILED(hr)) return AE_DUP_ACQUIRE_FRAME_FAILED;

		// if still holding old frame, destroy it
		if (_image)
		{
			_image->Release();
			_image = nullptr;
		}

		// QI for IDXGIResource
		hr = dxgi_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&_image));
		dxgi_res->Release();
		dxgi_res = nullptr;
		if (FAILED(hr)) return AE_DUP_QI_FRAME_FAILED;

		// copy old description
		D3D11_TEXTURE2D_DESC frame_desc;
		_image->GetDesc(&frame_desc);

		// create a new staging buffer for fill frame image
		ID3D11Texture2D *new_image = NULL;
		frame_desc.Usage = D3D11_USAGE_STAGING;
		frame_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		frame_desc.BindFlags = 0;
		frame_desc.MiscFlags = 0;
		frame_desc.MipLevels = 1;
		frame_desc.ArraySize = 1;
		frame_desc.SampleDesc.Count = 1;
		hr = _d3d_device->CreateTexture2D(&frame_desc, NULL, &new_image);
		if (FAILED(hr)) return AE_DUP_CREATE_TEXTURE_FAILED;


		// copy next staging buffer to new staging buffer
		_d3d_ctx->CopyResource(new_image, _image);
		

		// create staging buffer for map bits
		IDXGISurface *dxgi_surface = NULL;
		hr = new_image->QueryInterface(__uuidof(IDXGISurface), (void **)(&dxgi_surface));
		new_image->Release();
		if (FAILED(hr)) return AE_DUP_QI_DXGI_FAILED;

		// map buff to mapped rect structure
		DXGI_MAPPED_RECT mapped_rect;
		hr = dxgi_surface->Map(&mapped_rect, DXGI_MAP_READ);
		if (FAILED(hr)) return AE_DUP_MAP_FAILED;

		memcpy(_buffer, mapped_rect.pBits, _buffer_size);
		dxgi_surface->Unmap();

		dxgi_surface->Release();
		dxgi_surface = nullptr;

		return AE_NO;
	}

	int record_desktop_duplication::get_desktop_cursor(const DXGI_OUTDUPL_FRAME_INFO *frame_info)
	{
		// A non-zero mouse update timestamp indicates that there is a mouse position update and optionally a shape change
		if (frame_info->LastMouseUpdateTime.QuadPart == 0)
		{
			return AE_NO;
		}

		bool UpdatePosition = true;

		// Make sure we don't update pointer position wrongly
		// If pointer is invisible, make sure we did not get an update from another output that the last time that said pointer
		// was visible, if so, don't set it to invisible or update.
		if (!frame_info->PointerPosition.Visible && (_cursor_info.output_index != _output_index))
		{
			UpdatePosition = false;
		}

		// If two outputs both say they have a visible, only update if new update has newer timestamp
		if (frame_info->PointerPosition.Visible && _cursor_info.visible && (_cursor_info.output_index != _output_index) && (_cursor_info.pre_timestamp.QuadPart > frame_info->LastMouseUpdateTime.QuadPart))
		{
			UpdatePosition = false;
		}

		// Update position
		if (UpdatePosition)
		{
			_cursor_info.position.x = frame_info->PointerPosition.Position.x + _output_des.DesktopCoordinates.left;
			_cursor_info.position.y = frame_info->PointerPosition.Position.y + _output_des.DesktopCoordinates.top;
			_cursor_info.output_index = _output_index;
			_cursor_info.pre_timestamp = frame_info->LastMouseUpdateTime;
			_cursor_info.visible = frame_info->PointerPosition.Visible != 0;
		}

		// No new shape
		if (frame_info->PointerShapeBufferSize == 0)
		{
			return AE_NO;
		}

		// Old buffer too small
		if (frame_info->PointerShapeBufferSize > _cursor_info.size)
		{
			if (_cursor_info.buff)
			{
				delete[] _cursor_info.buff;
				_cursor_info.buff = nullptr;
			}
			_cursor_info.buff = new (std::nothrow) BYTE[frame_info->PointerShapeBufferSize];
			if (!_cursor_info.buff)
			{
				_cursor_info.size = 0;
				return AE_ALLOCATE_FAILED;
			}

			// Update buffer size
			_cursor_info.size = frame_info->PointerShapeBufferSize;
		}

		// Get shape
		UINT BufferSizeRequired;
		HRESULT hr = _duplication->GetFramePointerShape(frame_info->PointerShapeBufferSize, reinterpret_cast<VOID*>(_cursor_info.buff), &BufferSizeRequired, &(_cursor_info.shape));
		if (FAILED(hr))
		{
			delete[] _cursor_info.buff;
			_cursor_info.buff = nullptr;
			_cursor_info.size = 0;
			return AE_DUP_GET_CURSORSHAPE_FAILED;
		}

		return AE_NO;
	}

	void record_desktop_duplication::draw_cursor()
	{
		for (int h = 0; h < _cursor_info.shape.Height; h++) {
			memcpy(_buffer + (_cursor_info.position.y + h) *_width * 4 + _cursor_info.position.x * 4, _cursor_info.buff + _cursor_info.shape.Width *h * 4, _cursor_info.shape.Width * 4);
		}
	}

	void record_desktop_duplication::do_sleep(int64_t dur, int64_t pre, int64_t now)
	{
		int64_t delay = now - pre;
		dur = delay > dur ? max(0, dur - (delay - dur)) : (dur + dur - delay);

		//al_debug("%lld", delay);

		if (dur)
			av_usleep(dur);
	}

	void record_desktop_duplication::record_func()
	{
		AVFrame *frame = av_frame_alloc();
		int64_t pre_pts = 0, dur = AV_TIME_BASE / _fps;

		int error = AE_NO;

		if (attatch_desktop() != true) {
			al_fatal("duplication attach desktop failed :%lu",GetLastError());
			if (_on_error) _on_error(AE_DUP_ATTATCH_FAILED);
			return;
		}

		//should init after desktop attatched
		error = init_duplication();
		if (error != AE_NO) {
			al_fatal("duplication initialize failed %s,last error :%lu", err2str(error), GetLastError());
			if (_on_error) _on_error(error);
			return;
		}

		DXGI_OUTDUPL_FRAME_INFO frame_info;
		while (_running)
		{
			//timeout is no new picture,no need to update
			if ((error = get_desktop_image(&frame_info)) == AE_TIMEOUT) continue;

			if (error != AE_NO) {
				if (_on_error) _on_error(error);
				break;
			}

			if ((error = get_desktop_cursor(&frame_info)) == AE_NO)
				draw_cursor();

			free_duplicated_frame();

			frame->pts = av_gettime_relative();
			frame->pkt_dts = frame->pts;
			frame->pkt_pts = frame->pts;

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