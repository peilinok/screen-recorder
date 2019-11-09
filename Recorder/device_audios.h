#ifndef  RECORD_DEVICES
#define RECORD_DEVICES

#include <list>
#include <string>

namespace am {
	typedef struct {
		std::string id;
		std::string name;
		uint8_t is_default;
	}DEVICE_AUDIOS;

	class device_audios
	{
	public:
		static int get_default_input_device(std::string &id, std::string &name);

		static int get_default_ouput_device(std::string &id, std::string &name);

		static int get_input_devices(std::list<DEVICE_AUDIOS> &devices);

		static int get_output_devices(std::list<DEVICE_AUDIOS> &devices);

	private:
		static int get_devices(bool input, std::list<DEVICE_AUDIOS> &devices);

		static int get_default(bool input, std::string &id, std::string &name);
	};

}

#endif // ! RECORD_DEVICES