#include "export.h"

namespace am {
	class recorder {
	private:
		recorder();

		~recorder();

	public:
		static recorder *instance();

		static void release();

		int init(const AMRECORDER_SETTING & setting, const AMRECORDER_CALLBACK &callbacks);

		int start();

		void stop();

		int pause();

		int resume();

		int get_speakers(AMRECORDER_DEVICE **devices);

		int get_mics(AMRECORDER_DEVICE **devices);

		int get_cameras(AMRECORDER_DEVICE **devices);

	private:
		AMRECORDER_SETTING _setting;
		AMRECORDER_CALLBACK _callbacks;


	};

	static recorder *_instance = nullptr;

	recorder::recorder() {

	}

	recorder::~recorder() {

	}

	recorder * recorder::instance() {
		if (_instance == nullptr) _instance = new recorder();

		return _instance;
	}

	void recorder::release()
	{
		if (_instance)
			delete _instance;

		_instance = nullptr;
	}

	int recorder::init(const AMRECORDER_SETTING & setting, const AMRECORDER_CALLBACK & callbacks)
	{
		return 0;
	}

	int recorder::start()
	{
		return 0;
	}

	void recorder::stop()
	{
	}

	int recorder::pause()
	{
		return 0;
	}

	int recorder::resume()
	{
		return 0;
	}

	int recorder::get_speakers(AMRECORDER_DEVICE ** devices)
	{
		return 0;
	}

	int recorder::get_mics(AMRECORDER_DEVICE ** devices)
	{
		return 0;
	}

	int recorder::get_cameras(AMRECORDER_DEVICE ** devices)
	{
		return 0;
	}
}

AMRECORDER_API int recorder_init(const AMRECORDER_SETTING & setting, const AMRECORDER_CALLBACK & callbacks)
{
	return am::recorder::instance()->init(setting, callbacks);
}

AMRECORDER_API void recorder_release()
{
	return am::recorder::instance()->release();
}

AMRECORDER_API int recorder_start()
{
	return am::recorder::instance()->start();
}

AMRECORDER_API void recorder_stop()
{
	return am::recorder::instance()->stop();
}

AMRECORDER_API int recorder_pause()
{
	return am::recorder::instance()->pause();
}

AMRECORDER_API int recorder_resume()
{
	return am::recorder::instance()->resume();
}

AMRECORDER_API int recorder_get_speakers(AMRECORDER_DEVICE ** devices)
{
	return am::recorder::instance()->get_speakers(devices);
}

AMRECORDER_API int recorder_get_mics(AMRECORDER_DEVICE ** devices)
{
	return am::recorder::instance()->get_mics(devices);
}

AMRECORDER_API int recorder_get_cameras(AMRECORDER_DEVICE ** devices)
{
	return am::recorder::instance()->get_cameras(devices);
}
