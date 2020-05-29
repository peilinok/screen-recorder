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
			D3D_DRIVER_TYPE DriverTypes[] =
			{
				D3D_DRIVER_TYPE_HARDWARE,
				D3D_DRIVER_TYPE_WARP,
				D3D_DRIVER_TYPE_REFERENCE,
			};
			UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

			// Feature levels supported
			D3D_FEATURE_LEVEL FeatureLevels[] =
			{
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_1
			};
			UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

			D3D_FEATURE_LEVEL FeatureLevel;

			// Create device
			for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
			{
				hr = create_device(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
					D3D11_SDK_VERSION, &_d3d_device, &FeatureLevel, &_d3d_ctx);
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
			D3D11_INPUT_ELEMENT_DESC Layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			UINT NumElements = ARRAYSIZE(Layout);
			hr = _d3d_device->CreateInputLayout(Layout, NumElements, g_VS, Size, &_d3d_inlayout);
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
			D3D11_SAMPLER_DESC SampDesc;
			RtlZeroMemory(&SampDesc, sizeof(SampDesc));
			SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			SampDesc.MinLOD = 0;
			SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = _d3d_device->CreateSamplerState(&SampDesc, &_d3d_samplerlinear);
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

	int record_desktop_duplication::do_record()
	{
		IDXGIResource* DesktopResource = nullptr;
		DXGI_OUTDUPL_FRAME_INFO FrameInfo;

		// Get new frame
		HRESULT hr = _duplication->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) return AE_NO;

		if (FAILED(hr)) return AE_DUP_ACQUIRE_FRAME_FAILED;

		// If still holding old frame, destroy it
		if (_image)
		{
			_image->Release();
			_image = nullptr;
		}

		// QI for IDXGIResource
		hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&_image));
		DesktopResource->Release();
		DesktopResource = nullptr;
		if (FAILED(hr)) return AE_DUP_QI_FRAME_FAILED;

		//
		// copy old description
		//
		D3D11_TEXTURE2D_DESC frameDescriptor;
		_image->GetDesc(&frameDescriptor);

		//
		// create a new staging buffer for fill frame image
		//
		ID3D11Texture2D *hNewDesktopImage = NULL;
		frameDescriptor.Usage = D3D11_USAGE_STAGING;
		frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		frameDescriptor.BindFlags = 0;
		frameDescriptor.MiscFlags = 0;
		frameDescriptor.MipLevels = 1;
		frameDescriptor.ArraySize = 1;
		frameDescriptor.SampleDesc.Count = 1;
		hr = _d3d_device->CreateTexture2D(&frameDescriptor, NULL, &hNewDesktopImage);
		if (FAILED(hr))
		{
			free_duplicated_frame();
			return FALSE;
		}

		//
		// copy next staging buffer to new staging buffer
		//
		_d3d_ctx->CopyResource(hNewDesktopImage, _image);

		free_duplicated_frame();

		//
		// create staging buffer for map bits
		//
		IDXGISurface *hStagingSurf = NULL;
		hr = hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void **)(&hStagingSurf));
		hNewDesktopImage->Release();
		if (FAILED(hr))
		{
			return FALSE;
		}

		//
		// copy bits to user space
		//
		DXGI_MAPPED_RECT mappedRect;
		hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
		if (SUCCEEDED(hr))
		{
			memcpy(_buffer, mappedRect.pBits, _buffer_size);
			hStagingSurf->Unmap();
		}

		hStagingSurf->Release();
		hStagingSurf = nullptr;

		return AE_NO;
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

		
		while (_running)
		{
			error = do_record();
			if (error != AE_NO) {
				if (_on_error) _on_error(error);
				break;
			}

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