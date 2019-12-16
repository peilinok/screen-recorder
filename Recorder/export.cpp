#include "export.h"

#include "device_audios.h"

#include "record_audio_factory.h"
#include "record_desktop_factory.h"

#include "muxer_define.h"
#include "muxer_mp4.h"

#include "error_define.h"
#include "log_helper.h"

#include <string>
#include <atomic>
#include <mutex>

#define USE_DSHOW 0

namespace am {
	typedef std::lock_guard<std::mutex> lock_guard;

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

		record_audio *_recorder_speaker;
		record_audio *_recorder_mic;
		record_desktop *_recorder_desktop;

		muxer_mp4 *_muxer;

		std::atomic_bool _inited;
		std::mutex _mutex;
	};

	static recorder *_g_instance = nullptr;
	static std::mutex _g_mutex;

	recorder::recorder() {
		memset(&_setting, 0, sizeof(_setting));
		memset(&_callbacks, 0, sizeof(_callbacks));

		_recorder_speaker = nullptr;
		_recorder_mic = nullptr;
		_recorder_desktop = nullptr;

		_inited = false;
		_muxer = nullptr;
	}

	recorder::~recorder() {
		if (_muxer)
			delete _muxer;

		if (_recorder_desktop)
			delete _recorder_desktop;

		if (_recorder_mic)
			delete _recorder_mic;

		if (_recorder_speaker)
			delete _recorder_speaker;
	}

	recorder * recorder::instance() {
		lock_guard lock(_g_mutex);

		if (_g_instance == nullptr) _g_instance = new recorder();

		return _g_instance;
	}

	void recorder::release()
	{
		lock_guard lock(_g_mutex);

		if (_g_instance)
			delete _g_instance;

		_g_instance = nullptr;
	}

	int recorder::init(const AMRECORDER_SETTING & setting, const AMRECORDER_CALLBACK & callbacks)
	{
		lock_guard lock(_mutex);
		if (_inited == true)
			return AE_NO;

		int error = AE_NO;

		_setting = setting;
		_callbacks = callbacks;

#if USE_DSHOW

		error = record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_speaker);
		AMERROR_CHECK(error);

		error = _recorder_speaker->init("audio=virtual-audio-capturer", "audio=virtual-audio-capturer", false);
		AMERROR_CHECK(error);

		error = record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_mic);
		AMERROR_CHECK(error);

		error = _recorder_mic->init(std::string("audio=") + std::string(setting.a_mic.name), std::string("audio=") + std::string(setting.a_mic.name), true);
		AMERROR_CHECK(error);

		error = record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_DSHOW, &_recorder_desktop);
		AMERROR_CHECK(error);
#else
		error = record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_speaker);
		AMERROR_CHECK(error);

		error = _recorder_speaker->init(setting.a_speaker.name, setting.a_speaker.id, false);
		AMERROR_CHECK(error);

		error = record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_mic);
		AMERROR_CHECK(error);

		error = _recorder_mic->init(setting.a_mic.name, setting.a_mic.id, true);
		AMERROR_CHECK(error);

		error = record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_GDI, &_recorder_desktop);
		AMERROR_CHECK(error);
#endif 

		error = _recorder_desktop->init(
		{ 
			setting.v_left,setting.v_top,setting.v_width,setting.v_height 
		}, 
			setting.v_frame_rate
		);
		AMERROR_CHECK(error);

		am::MUX_SETTING mux_setting;
		mux_setting.v_frame_rate = setting.v_frame_rate;
		mux_setting.v_bit_rate = setting.v_bit_rate;
		mux_setting.v_width = setting.v_width;
		mux_setting.v_height = setting.v_height;
		mux_setting.v_qb = 60;

		mux_setting.a_nb_channel = 2;
		mux_setting.a_sample_fmt = AV_SAMPLE_FMT_FLTP;
		mux_setting.a_sample_rate = 44100;
		mux_setting.a_bit_rate = 128000;

#if USE_DSHOW
		am::record_audio *audios[2] = { _recorder_speaker,_recorder_mic };
#else
		am::record_audio *audios[2] = { _recorder_mic,_recorder_speaker };
#endif

		_muxer = new muxer_mp4();
		error = _muxer->init(setting.output, _recorder_desktop, audios, 2, mux_setting);
		AMERROR_CHECK(error);

		_inited = true;

		return error;
	}

	int recorder::start()
	{
		lock_guard lock(_mutex);
		if (_inited == false)
			return AE_NEED_INIT;

		int error = _muxer->start();

		return error;
	}

	void recorder::stop()
	{
		lock_guard lock(_mutex);
		if (_inited == false)
			return;

		_muxer->stop();
	}

	int recorder::pause()
	{
		lock_guard lock(_mutex);
		if (_inited == false)
			return AE_NEED_INIT;

		return _muxer->pause();
	}

	int recorder::resume()
	{
		lock_guard lock(_mutex);
		if (_inited == false)
			return AE_NEED_INIT;

		return _muxer->resume();
	}

	int recorder::get_speakers(AMRECORDER_DEVICE ** devices)
	{
		std::list<am::DEVICE_AUDIOS> device_list;

		int error = am::device_audios::get_output_devices(device_list);
		if (error != AE_NO) return -error;

		int count = device_list.size();

		*devices = new AMRECORDER_DEVICE[count];

		int index = 0;
		for each (auto device in device_list)
		{
			al_info("audio input name:%s id:%s", device.name.c_str(), device.id.c_str());

			(*devices)[index].is_default = device.is_default;
			sprintf_s((*devices)[index].id, 260, "%s", device.id.c_str());
			sprintf_s((*devices)[index].name, 260, "%s", device.name.c_str());

			index++;
		}

		return count;
	}

	int recorder::get_mics(AMRECORDER_DEVICE ** devices)
	{
		std::list<am::DEVICE_AUDIOS> device_list;

		int error = am::device_audios::get_input_devices(device_list);
		if (error != AE_NO) return -error;

		int count = device_list.size();

		*devices = new AMRECORDER_DEVICE[count];

		int index = 0;
		for each (auto device in device_list)
		{
			al_info("audio output name:%s id:%s", device.name.c_str(), device.id.c_str());
			
			(*devices)[index].is_default = device.is_default;
			sprintf_s((*devices)[index].id, 260, "%s", device.id.c_str());
			sprintf_s((*devices)[index].name, 260, "%s", device.name.c_str());

			index++;
		}

		return count;
	}

	int recorder::get_cameras(AMRECORDER_DEVICE ** devices)
	{
		return -AE_UNSUPPORT;
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
