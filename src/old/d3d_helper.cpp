#include "d3d_helper.h"

#include "system_lib.h"

#include "error_define.h"

namespace am {
	typedef HRESULT(WINAPI *DXGI_FUNC_CREATEFACTORY)(REFIID, IDXGIFactory1 **);

	std::list<IDXGIAdapter*> d3d_helper::get_adapters(int * error, bool free_lib)
	{
		std::list<IDXGIAdapter*> adapters;

		*error = AE_NO;

		HMODULE hdxgi = load_system_library("dxgi.dll");

		if (!hdxgi) {
			*error = AE_D3D_LOAD_FAILED;
			return adapters;
		}

		do {
			DXGI_FUNC_CREATEFACTORY create_factory = nullptr;
			create_factory = (DXGI_FUNC_CREATEFACTORY)GetProcAddress(hdxgi, "CreateDXGIFactory1");

			if (create_factory == nullptr) {
				*error = AE_DXGI_GET_PROC_FAILED;
				break;
			}

			IDXGIFactory1 * dxgi_factory = nullptr;
			HRESULT hr = create_factory(__uuidof(IDXGIFactory1), &dxgi_factory);
			if (FAILED(hr)) {
				*error = AE_DXGI_GET_FACTORY_FAILED;
				break;
			}

			unsigned int i = 0;
			IDXGIAdapter *adapter = nullptr;
			while (dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
			{
				if(adapter)
					adapters.push_back(adapter);
				++i;
			}

			dxgi_factory->Release();

		} while (0);

		if (free_lib && hdxgi) free_system_library(hdxgi);

		return adapters;
	}
}