#ifndef  RECORD_DEVICES
#define RECORD_DEVICES

#include <list>

namespace am {

	class record_devices
	{
	public:
		record_devices();
		~record_devices();

	public:
		int get_input_devices(std::list<std::string> devices);
	};

}

#endif // ! RECORD_DEVICES