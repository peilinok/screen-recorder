#ifndef HARDWARE_ACCELERATION
#define HARDWARE_ACCELERATION

#include <string>
#include <vector>
#include <list>

#define ENCODER_NAME_LEN 100

namespace am {
	typedef enum _HARDWARE_TYPE {
		HARDWARE_TYPE_UNKNOWN,
		HARDWARE_TYPE_NVENC,
		HARDWARE_TYPE_QSV,
		HARDWARE_TYPE_AMF,
		HARDWARE_TYPE_VAAPI
	}HARDWARE_TYPE;

	typedef struct _HARDWARE_ENCODER {
		HARDWARE_TYPE type;
		char name[ENCODER_NAME_LEN];
	}HARDWARE_ENCODER;

	class hardware_acceleration
	{
	private:
		hardware_acceleration(){}
		~hardware_acceleration(){}

	public:
		static std::vector<std::string> get_video_hardware_devices();
		static std::list<HARDWARE_ENCODER> get_supported_video_encoders();
	};

}

#endif