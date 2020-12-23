#include "ray_audio_device.h"

#include "include\ray_base.h"

#include "constants\ray_error_str.h"

#include "utils\win\ray_com.h"
#include "utils\ray_string.h"
#include "utils\ray_log.h"

#if defined(WIN32)

namespace ray {
namespace devices {

int AudioDeviceManager::setMicrophone(const char id[kAdmDeviceIdSize])
{
	return 0;
}

int AudioDeviceManager::setMicrophoneVolume(const unsigned int volume)
{

	return 0;
}

int AudioDeviceManager::getMicrophoneVolume(unsigned int & volume)
{

	return 0;
}

ray_refptr<IAudioDeviceCollection> AudioDeviceManager::getMicrophoneCollection()
{
	std::list<AudioDeviceInfo> devices;
	if (getAudioDevices(true, devices) == ERR_NO) {
		return new RefCountedObject<AudioDeviceCollection>(devices);
	}

	return nullptr;
}

int AudioDeviceManager::setSpeaker(const char id[kAdmDeviceIdSize])
{
	return 0;
}

int AudioDeviceManager::setSpeakerVolume(const unsigned int volume)
{

	return 0;
}

int AudioDeviceManager::getSpeakerVolume(unsigned int & volume)
{


	return 0;
}

ray_refptr<IAudioDeviceCollection> AudioDeviceManager::getSpeakerCollection()
{
	std::list<AudioDeviceInfo> devices;
	if (getAudioDevices(false, devices) == ERR_NO) {
		return new RefCountedObject<AudioDeviceCollection>(devices);
	}

	return nullptr;
}

int AudioDeviceManager::getAudioDevices(bool capture, std::list<AudioDeviceInfo>& devices)
{
	utils::com_initialize com_obj;

	Microsoft::WRL::ComPtr<IMMDeviceEnumerator> device_enumerator = nullptr;
	Microsoft::WRL::ComPtr<IMMDevice> device = nullptr;
	Microsoft::WRL::ComPtr<IMMDeviceCollection> collection = nullptr;

	LPWSTR current_device_id = NULL;

	int ret = ERR_NO;

	devices.clear();

	do {
		std::shared_ptr<void> raii_ptr(nullptr, [&](void*) {
			collection = nullptr;
			device = nullptr;
			device_enumerator = nullptr;
		});

		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator),
			(void**)device_enumerator.GetAddressOf());

		if (FAILED(hr)) {
			ret = ERR_CO_CREATE_FAILED;
			break;
		}

		hr = device_enumerator->GetDefaultAudioEndpoint(
			capture == true ? eCapture : eRender,
			eConsole,
			device.GetAddressOf());

		if (FAILED(hr)) {
			ret = ERR_CO_GETENDPOINT_FAILED;
			break;
		}

		hr = device_enumerator->EnumAudioEndpoints(
			capture == true ? eCapture : eRender,
			DEVICE_STATE_ACTIVE,
			collection.GetAddressOf());

		if (FAILED(hr)) {
			ret = ERR_CO_ENUMENDPOINT_FAILED;
			break;
		}

		UINT count;
		hr = collection->GetCount(&count);
		if (FAILED(hr)) {
			ret = ERR_CO_GET_ENDPOINT_COUNT_FAILED;
			break;
		}

		hr = device->GetId(&current_device_id);
		if (FAILED(hr)) {
			ret = ERR_CO_GET_ENDPOINT_ID_FAILED;
			break;
		}

		std::string default_id = utils::strings::unicode_utf8(current_device_id);

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

			/*
			hr = pEndpointDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_INPROC_SERVER, NULL,
			(LPVOID*)&deviceTopology);
			hr = deviceTopology->GetConnector(0, &connector);

			hr = connector->GetConnectorIdConnectedTo(&device_name);

			str_name = utils::strings::unicode_utf8(device_name);
			*/

			hr = pEndpointDevice->GetId(&device_id);
			if (FAILED(hr)) continue;

			str_id = utils::strings::unicode_utf8(device_id);

			hr = pEndpointDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
			if (FAILED(hr)) continue;

			hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
			if (FAILED(hr)) {
				PropVariantClear(&pv);
				continue;
			}

			if (pv.vt == VT_LPWSTR) {
				str_friendly = utils::strings::unicode_utf8(pv.pwszVal);
			}
			else if (pv.vt == VT_LPSTR) {
				str_friendly = utils::strings::ascii_utf8(pv.pszVal);
			}

			AudioDeviceInfo device_info;

			device_info.isMicrophone = capture;
			device_info.isCurrentSelected = device_info.isDefaultSelecteed = (str_id.compare(default_id) == 0);

			memcpy_s(device_info.name, kAdmDeviceNameSize, str_friendly.c_str(), str_friendly.length());
			memcpy_s(device_info.id, kAdmDeviceIdSize, str_id.c_str(), str_id.length());

			devices.emplace_back(device_info);

			PropVariantClear(&pv);
			CoTaskMemFree(device_name);
			CoTaskMemFree(device_id);
		}
	} while (0);


	if (ret != ERR_NO)
		LOG(ERROR) << "get audio devices failed(" << GetLastError() << "): " << (err2str(ret));

	return ret;

}

} // devices
} // ray


#endif // _WIN32
