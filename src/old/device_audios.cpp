#include "device_audios.h"

#include <memory>

#include "error_define.h"
#include "utils\ray_string.h"
#include "headers_mmdevice.h"
#include "utils\ray_log.h"

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

		int ret = AE_NO;

		do {
			std::shared_ptr<void> raii_ptr(nullptr, [&](void*) {
				collection = nullptr;
				device = nullptr;
				device_enumerator = nullptr;
			});

			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
				__uuidof(IMMDeviceEnumerator), (void**)device_enumerator.GetAddressOf());

			if (FAILED(hr)) {
				ret = AE_CO_CREATE_FAILED;
				break;
			}

			hr = device_enumerator->GetDefaultAudioEndpoint(input == true ? eCapture : eRender, eConsole, device.GetAddressOf());
			if (FAILED(hr)) {
				ret = AE_CO_GETENDPOINT_FAILED;
				break;
			}

			hr = device_enumerator->EnumAudioEndpoints(input == true ? eCapture : eRender, DEVICE_STATE_ACTIVE, collection.GetAddressOf());
			if (FAILED(hr)) {
				ret = AE_CO_ENUMENDPOINT_FAILED;
				break;
			}

			UINT count;
			hr = collection->GetCount(&count);
			if (FAILED(hr)) {
				ret = AE_CO_GET_ENDPOINT_COUNT_FAILED;
				break;
			}

			hr = device->GetId(&current_device_id);
			if (FAILED(hr)) {
				ret = AE_CO_GET_ENDPOINT_ID_FAILED;
				break;
			}

			std::string default_id = ray::utils::strings::unicode_utf8(current_device_id);

			CoTaskMemFree(current_device_id);

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
				if (FAILED(hr)) continue;

				/*hr = pEndpointDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_INPROC_SERVER, NULL,
					(LPVOID*)&deviceTopology);
				hr = deviceTopology->GetConnector(0, &connector);

				hr = connector->GetConnectorIdConnectedTo(&device_name);

				str_name = ray::utils::strings::unicode_utf8(device_name);*/

				hr = pEndpointDevice->GetId(&device_id);
				if (FAILED(hr)) continue;

				str_id = ray::utils::strings::unicode_utf8(device_id);

				hr = pEndpointDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
				if (FAILED(hr)) continue;

				hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
				if (FAILED(hr)) {
					PropVariantClear(&pv);
					continue;
				}

				if (pv.vt == VT_LPWSTR) {
					str_friendly = ray::utils::strings::unicode_utf8(pv.pwszVal);
				}
				else if (pv.vt == VT_LPSTR) {
					str_friendly = ray::utils::strings::ascii_utf8(pv.pszVal);
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
		} while (0);

		if (ret == AE_NO && devices.size()) {
			devices.push_front({
				ray::utils::strings::ascii_utf8(DEFAULT_AUDIO_INOUTPUT_ID),
				ray::utils::strings::ascii_utf8(DEFAULT_AUDIO_INOUTPUT_NAME),
				true
			});
		}


		if (ret != AE_NO)
			LOG(ERROR) << "get audio devices failed(" << GetLastError() << "): " << (err2str(ret));

		return ret;
	}

	int device_audios::get_default(bool input, std::string & id, std::string & name)
	{
		com_initialize com_obj;

		Microsoft::WRL::ComPtr<IMMDeviceEnumerator> device_enumerator = nullptr;
		Microsoft::WRL::ComPtr<IMMDevice> device = nullptr;
		Microsoft::WRL::ComPtr<IMMDeviceCollection> collection = nullptr;
		LPWSTR current_device_id = NULL;

		std::shared_ptr<void> raii_ptr(nullptr, [&](void*) {
			collection = nullptr;
			device = nullptr;
			device_enumerator = nullptr;
		});

		int ret = AE_NO;
		do {

			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
				__uuidof(IMMDeviceEnumerator), (void**)device_enumerator.GetAddressOf());
			if (FAILED(hr)) {
				ret = AE_CO_CREATE_FAILED;
				break;
			}

			hr = device_enumerator->GetDefaultAudioEndpoint(input == true ? eCapture : eRender, eConsole, device.GetAddressOf());
			if (FAILED(hr)) {
				ret = AE_CO_GETENDPOINT_FAILED;
				break;
			}

			hr = device_enumerator->EnumAudioEndpoints(input == true ? eCapture : eRender, DEVICE_STATE_ACTIVE, collection.GetAddressOf());
			if (FAILED(hr)) {
				ret = AE_CO_ENUMENDPOINT_FAILED;
				break;
			}

			UINT count;
			hr = collection->GetCount(&count);
			if (FAILED(hr)) {
				ret = AE_CO_GET_ENDPOINT_COUNT_FAILED;
				break;
			}

			hr = device->GetId(&current_device_id);
			if (FAILED(hr)) {
				ret = AE_CO_GET_ENDPOINT_ID_FAILED;
				break;
			}

			id = ray::utils::strings::unicode_utf8(current_device_id);

			CoTaskMemFree(current_device_id);

			IPropertyStore *pPropertyStore = NULL;
			PROPVARIANT pv;
			PropVariantInit(&pv);

			hr = device->OpenPropertyStore(STGM_READ, &pPropertyStore);
			if (FAILED(hr)) {
				ret = AE_CO_OPEN_PROPERTY_FAILED;
				break;
			}

			hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
			if (FAILED(hr)) {
				ret = AE_CO_GET_VALUE_FAILED;
				break;
			}

			if (pv.vt == VT_LPWSTR) {
				name = ray::utils::strings::unicode_utf8(pv.pwszVal);
			}
			else if (pv.vt == VT_LPSTR) {
				name = ray::utils::strings::ascii_utf8(pv.pszVal);
			}

			PropVariantClear(&pv);
		} while (0);


		if (ret != AE_NO)
			LOG(ERROR) << "get default audio device failed(" << GetLastError() << "): " << (err2str(ret));

		return ret;
	}

}