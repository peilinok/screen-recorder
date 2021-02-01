#pragma once

#include "utils\win\ray_com.h"

namespace ray {
namespace devices {


class WasapiDeviceEnumerator
	:public IMMNotificationClient
{
public:
	WasapiDeviceEnumerator();

private:
	~WasapiDeviceEnumerator();

	DISALLOW_COPY_AND_ASSIGN(WasapiDeviceEnumerator);

public:

	int Initialize();

	ULONG STDMETHODCALLTYPE AddRef() override;

	ULONG STDMETHODCALLTYPE Release() override;

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override;

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(_In_  LPCWSTR pwstrDeviceId, _In_  DWORD dwNewState) override;

	HRESULT STDMETHODCALLTYPE OnDeviceAdded(_In_  LPCWSTR pwstrDeviceId) override;

	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(_In_  LPCWSTR pwstrDeviceId) override;

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(_In_  EDataFlow flow, _In_  ERole role, _In_  LPCWSTR pwstrDefaultDeviceId) override;

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(_In_  LPCWSTR pwstrDeviceId, _In_  const PROPERTYKEY key) override;

private:
	
	HRESULT _PrintDeviceName(LPCWSTR  pwstrId);

private:

	long ref_count_;

	Microsoft::WRL::ComPtr<IMMDeviceEnumerator> ptr_enumerator_;

	utils::com_initialize com_obj;
};

}
}