#include "ray_audio_wasapi.h"

#include "utils\ray_string.h"
#include "utils\ray_log.h"

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

namespace ray {
namespace devices {

WasapiDeviceEnumerator::WasapiDeviceEnumerator()
	:ref_count_(1), ptr_enumerator_(nullptr)
{
	
}

WasapiDeviceEnumerator::~WasapiDeviceEnumerator()
{
	if (ptr_enumerator_)
		ptr_enumerator_->UnregisterEndpointNotificationCallback(this);
}


int WasapiDeviceEnumerator::Initialize()
{
	HRESULT hr = S_OK;

	if (ptr_enumerator_ == nullptr)
	{
		// Get enumerator for audio endpoint devices.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IMMDeviceEnumerator), (void**)ptr_enumerator_.GetAddressOf());
	}

	ptr_enumerator_->RegisterEndpointNotificationCallback(this);

	return 0;
}

ULONG WasapiDeviceEnumerator::AddRef()
{
	return InterlockedIncrement(&ref_count_);
}

ULONG WasapiDeviceEnumerator::Release()
{
	ULONG ulRef = InterlockedDecrement(&ref_count_);
	if (0 == ulRef)
	{
		delete this;
	}
	return ulRef;
}

HRESULT WasapiDeviceEnumerator::QueryInterface(REFIID riid, VOID ** ppvInterface)
{
	if (IID_IUnknown == riid)
	{
		AddRef();
		*ppvInterface = (IUnknown*)this;
	}
	else if (__uuidof(IMMNotificationClient) == riid)
	{
		AddRef();
		*ppvInterface = (IMMNotificationClient*)this;
	}
	else
	{
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}

	return S_OK;
}

HRESULT WasapiDeviceEnumerator::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{

	std::wstring pszState;

	_PrintDeviceName(pwstrDeviceId);

	switch (dwNewState)
	{
	case DEVICE_STATE_ACTIVE:
		pszState = L"ACTIVE";
		break;
	case DEVICE_STATE_DISABLED:
		pszState = L"DISABLED";
		break;
	case DEVICE_STATE_NOTPRESENT:
		pszState = L"NOTPRESENT";
		break;
	case DEVICE_STATE_UNPLUGGED:
		pszState = L"UNPLUGGED";
		break;
	}

	wprintf(L"  -->New device state is DEVICE_STATE_%s (0x%8.8x)\n",
		pszState.c_str(), dwNewState);

	return S_OK;
}

HRESULT WasapiDeviceEnumerator::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
	_PrintDeviceName(pwstrDeviceId);

	wprintf(L"  -->Added device\n");
	return S_OK;
}

HRESULT WasapiDeviceEnumerator::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
	return E_NOTIMPL;
}

HRESULT WasapiDeviceEnumerator::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
	std::wstring  pszFlow;
	std::wstring  pszRole;

	_PrintDeviceName(pwstrDefaultDeviceId);

	switch (flow)
	{
	case eRender:
		pszFlow = L"eRender";
		break;
	case eCapture:
		pszFlow = L"eCapture";
		break;
	}

	switch (role)
	{
	case eConsole:
		pszRole = L"eConsole";
		break;
	case eMultimedia:
		pszRole = L"eMultimedia";
		break;
	case eCommunications:
		pszRole = L"eCommunications";
		break;
	}

	wprintf(L"  -->New default device: flow = %s, role = %s\n",
		pszFlow.c_str(), pszRole.c_str());
	return S_OK;
}

HRESULT WasapiDeviceEnumerator::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
	_PrintDeviceName(pwstrDeviceId);

	printf("  -->Changed device property "
		"{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
		key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
		key.fmtid.Data4[0], key.fmtid.Data4[1],
		key.fmtid.Data4[2], key.fmtid.Data4[3],
		key.fmtid.Data4[4], key.fmtid.Data4[5],
		key.fmtid.Data4[6], key.fmtid.Data4[7],
		key.pid);
	return S_OK;
}

HRESULT WasapiDeviceEnumerator::_PrintDeviceName(LPCWSTR pwstrId)
{
	HRESULT hr = S_OK;
	IMMDevice *pDevice = NULL;
	IPropertyStore *pProps = NULL;
	PROPVARIANT varString;

	PropVariantInit(&varString);

	if (ptr_enumerator_ == NULL)
	{
		// Get enumerator for audio endpoint devices.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
			NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IMMDeviceEnumerator),
			(void**)ptr_enumerator_.GetAddressOf());
	}
	if (hr == S_OK)
	{
		hr = ptr_enumerator_->GetDevice(pwstrId, &pDevice);
	}
	if (hr == S_OK)
	{
		hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
	}
	if (hr == S_OK)
	{
		// Get the endpoint device's friendly-name property.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
	}
	printf("----------------------\nDevice name: \"%s\"\n  Endpoint ID string: \"%s\"\n",
		(hr == S_OK) ? utils::strings::unicode_ascii(varString.pwszVal).c_str() : "null device",
		(pwstrId != NULL) ? utils::strings::unicode_ascii(pwstrId).c_str() : "null ID");

	PropVariantClear(&varString);

	SAFE_RELEASE(pProps);
	SAFE_RELEASE(pDevice);

	return hr;
}




}

}


