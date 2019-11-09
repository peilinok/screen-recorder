#include "device_audios.h"

#include "error_define.h"
#include "log_helper.h"

#include "utils_string.h"

#include "mmdevice_define.h"

#include <propkeydef.h>//must include before functiondiscoverykeys_devpkey
#include <functiondiscoverykeys_devpkey.h>

#include <wrl/client.h>
#include <devicetopology.h>

#include <memory>

namespace am {

	int device_audios::get_input_devices(std::list<DEVICE_AUDIOS>& devices)
	{
		return get_devices(true, devices);
	}

	int device_audios::get_output_devices(std::list<DEVICE_AUDIOS>& devices)
	{
		return get_devices(false, devices);
	}

	int device_audios::get_default_input_device(std::string & id, std::string & name)
	{
		return get_default(true, id, name);
	}

	int device_audios::get_default_ouput_device(std::string & id, std::string & name)
	{
		return get_default(false, id, name);
	}

	int device_audios::get_devices(bool input, std::list<DEVICE_AUDIOS>& devices)
	{
		com_initialize com_obj;

		devices.clear();

		Microsoft::WRL::ComPtr<IMMDeviceEnumerator> device_enumerator = nullptr;
		Microsoft::WRL::ComPtr<IMMDevice> device = nullptr;
		Microsoft::WRL::ComPtr<IMMDeviceCollection> collection = nullptr;
		LPWSTR current_device_id = NULL;

		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

		std::shared_ptr<void> raii_ptr(nullptr, [&](void*) {
			collection = nullptr;
			device = nullptr;
			device_enumerator = nullptr;
			CoUninitialize();
		});
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), (void**)device_enumerator.GetAddressOf());
		hr = device_enumerator->GetDefaultAudioEndpoint(input == true ? eCapture : eRender, eConsole, device.GetAddressOf());
		hr = device_enumerator->EnumAudioEndpoints(input == true ? eCapture : eRender, DEVICE_STATE_ACTIVE, collection.GetAddressOf());
		UINT count;
		hr = collection->GetCount(&count);

		device->GetId(&current_device_id);
		
		std::string default_id = utils_string::unicode_ascii(current_device_id);

		CoTaskMemFree(current_device_id);

		// 打印音频设备个数

		for (int i = 0; i < count; ++i) {
			IMMDevice* pEndpointDevice = NULL;
			IDeviceTopology* deviceTopology = NULL;
			IConnector* connector = NULL;

			IPropertyStore *pPropertyStore = NULL;
			PROPVARIANT pv;
			PropVariantInit(&pv);

			LPWSTR device_name = NULL;
			LPWSTR device_id = NULL;

			std::string str_name, str_id, str_friendly;

			hr = collection->Item(i, &pEndpointDevice);
			hr = pEndpointDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_INPROC_SERVER, NULL,
				(LPVOID*)&deviceTopology);
			hr = deviceTopology->GetConnector(0, &connector);

			hr = connector->GetConnectorIdConnectedTo(&device_name);

			str_name = utils_string::unicode_ascii(device_name);

			hr = pEndpointDevice->GetId(&device_id);

			str_id = utils_string::unicode_ascii(device_id);

			pEndpointDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);

			pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);

			if (pv.vt == VT_LPWSTR) {
				str_friendly = utils_string::unicode_ascii(pv.pwszVal);
			}
			else if (pv.vt == VT_LPSTR) {
				str_friendly = pv.pszVal;
			}

			devices.push_back({
				str_id,
				str_friendly,
				str_id.compare(default_id) == 0
			});

			PropVariantClear(&pv);
			CoTaskMemFree(device_name);
			CoTaskMemFree(device_id);
		}

		return AE_NO;
	}

	int device_audios::get_default(bool input, std::string & id, std::string & name)
	{
		com_initialize com_obj;

		Microsoft::WRL::ComPtr<IMMDeviceEnumerator> device_enumerator = nullptr;
		Microsoft::WRL::ComPtr<IMMDevice> device = nullptr;
		Microsoft::WRL::ComPtr<IMMDeviceCollection> collection = nullptr;
		LPWSTR current_device_id = NULL;

		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

		std::shared_ptr<void> raii_ptr(nullptr, [&](void*) {
			collection = nullptr;
			device = nullptr;
			device_enumerator = nullptr;
			CoUninitialize();
		});
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), (void**)device_enumerator.GetAddressOf());
		hr = device_enumerator->GetDefaultAudioEndpoint(input == true ? eCapture : eRender, eConsole, device.GetAddressOf());
		hr = device_enumerator->EnumAudioEndpoints(input == true ? eCapture : eRender, DEVICE_STATE_ACTIVE, collection.GetAddressOf());
		UINT count;

		hr = collection->GetCount(&count);

		device->GetId(&current_device_id);

		id = utils_string::unicode_ascii(current_device_id);

		CoTaskMemFree(current_device_id);

		IPropertyStore *pPropertyStore = NULL;
		PROPVARIANT pv;
		PropVariantInit(&pv);

		device->OpenPropertyStore(STGM_READ, &pPropertyStore);

		pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);

		if (pv.vt == VT_LPWSTR) {
			name = utils_string::unicode_ascii(pv.pwszVal);
		}
		else if (pv.vt == VT_LPSTR) {
			name = pv.pszVal;
		}

		return AE_NO;
	}

}