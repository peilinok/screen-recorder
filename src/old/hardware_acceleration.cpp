#include "hardware_acceleration.h"

#include <map>
#include <algorithm>

#include "headers_ffmpeg.h"

#include "d3d_helper.h"
#include "error_define.h"
#include "utils\ray_string.h"
#include "utils\ray_log.h"


namespace am {

	static const std::map<_HARDWARE_TYPE, const char*> encoder_map = {
		{ HARDWARE_TYPE_NVENC , "Nvidia.NVENC" },
		{ HARDWARE_TYPE_QSV , "Intel.QSV" },
		{ HARDWARE_TYPE_AMF , "AMD.AMF" },
		{ HARDWARE_TYPE_VAAPI , "FFmpeg.Vaapi"}
	};

	static const std::list<std::string> nvenc_blacklist = {
		"720M", "730M",  "740M",  "745M",  "820M",  "830M",
		"840M", "845M",  "920M",  "930M",  "940M",  "945M",
		"1030", "MX110", "MX130", "MX150", "MX230", "MX250",
		"M520", "M500",  "P500",  "K620M"
	};
	

	static bool get_encoder_name(HARDWARE_TYPE type, char name[ENCODER_NAME_LEN]);

	static bool is_nvenc_blacklist(std::string desc);

	static bool is_nvenc_canload();

	static bool is_nvenc_support();

	static bool is_qsv_support();

	static bool is_amf_support();

	static bool is_vaapi_support();

	bool get_encoder_name(HARDWARE_TYPE type, char name[ENCODER_NAME_LEN]) {

		if (encoder_map.find(type) == encoder_map.end()) return false;

		strcpy_s(name, ENCODER_NAME_LEN, ray::utils::strings::ascii_utf8(encoder_map.at(type)).c_str());

		return true;
	}

	bool is_nvenc_blacklist(std::string desc) {
		for (auto itr = nvenc_blacklist.begin(); itr != nvenc_blacklist.end(); itr++) {
			if (desc.find((*itr).c_str()) != std::string::npos)
				return true;
		}

		return false;
	}

	bool is_nvenc_canload() {		
		std::string module_name;
		if (sizeof(void *) == 8) {
			module_name = "nvEncodeAPI64.dll";
		}
		else {
			module_name = "nvEncodeAPI.dll";
		}

		HMODULE hnvenc = GetModuleHandleA(module_name.c_str());
		if (!hnvenc)
			hnvenc = LoadLibraryA(module_name.c_str());


		bool is_canload = !!hnvenc;

		if (hnvenc) FreeModule(hnvenc);

		return is_canload;
	}

	bool is_nvenc_support() {
		bool is_support = false;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
		av_register_all();
#endif
		do {
			if (avcodec_find_encoder_by_name("nvenc_h264") == nullptr &&
				avcodec_find_encoder_by_name("h264_nvenc") == nullptr)
				break;

#if defined(_WIN32)
			int error = AE_NO;
			auto adapters = d3d_helper::get_adapters(&error);
			if (error != AE_NO || adapters.size() == 0)
				break;

			bool has_device = false;
			for (std::list<IDXGIAdapter *>::iterator itr = adapters.begin(); itr != adapters.end(); itr++) {
				IDXGIOutput *adapter_output = nullptr;
				DXGI_ADAPTER_DESC adapter_desc = { 0 };
				DXGI_OUTPUT_DESC adapter_output_desc = { 0 };

				HRESULT hr = (*itr)->GetDesc(&adapter_desc);
				
				std::string strdesc = ray::utils::strings::unicode_ascii(adapter_desc.Description);
				std::transform(strdesc.begin(), strdesc.end(), strdesc.begin(), ::toupper);

				if (SUCCEEDED(hr) && (strdesc.find("NVIDIA") != std::string::npos) && !is_nvenc_blacklist(strdesc)) {
					has_device = true;
					break;
				}
			}

			if(!has_device) break;
			
			if (!is_nvenc_canload()) break;
#else
			/*
			if (!os_dlopen("libnvidia-encode.so.1"))
				break;
				*/
#endif

			is_support = true;
		} while (0);

		return is_support;
	}

	bool is_qsv_support() {
		bool is_support = false;

		return is_support;
	}

	bool is_amf_support() {
		bool is_support = false;

		return is_support;
	}

	bool is_vaapi_support() {
		bool is_support = false;

		return is_support;
	}

	std::vector<std::string> hardware_acceleration::get_video_hardware_devices() {
		std::vector<std::string> devices;

		enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;

		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
			devices.push_back(av_hwdevice_get_type_name(type));
		}

		
		AVCodec *nvenc = avcodec_find_encoder_by_name("nvenc_h264");
		if(nvenc == nullptr)
			nvenc = avcodec_find_encoder_by_name("h264_nvenc");

		if (nvenc)
			VLOG(VLOG_DEBUG) << "nvenc support";

		AVCodec *vaapi = avcodec_find_encoder_by_name("h264_qsv");
		if (vaapi)
			VLOG(VLOG_DEBUG) << "qsv support";

		return devices;
	}

	std::list<HARDWARE_ENCODER> hardware_acceleration::get_supported_video_encoders() {
		std::list<HARDWARE_ENCODER> encoders;

		HARDWARE_ENCODER encoder;

		encoder.type = HARDWARE_TYPE_NVENC;
		if (is_nvenc_support() && get_encoder_name(encoder.type, encoder.name)) {
			encoders.push_back(encoder);
		}

		encoder.type = HARDWARE_TYPE_QSV;
		if (is_qsv_support() && get_encoder_name(encoder.type, encoder.name)) {
			encoders.push_back(encoder);
		}

		encoder.type = HARDWARE_TYPE_AMF;
		if (is_amf_support() && get_encoder_name(encoder.type, encoder.name)) {
			encoders.push_back(encoder);
		}

		encoder.type = HARDWARE_TYPE_VAAPI;
		if (is_vaapi_support() && get_encoder_name(encoder.type, encoder.name)) {
			encoders.push_back(encoder);
		}


		return encoders;
	}

}