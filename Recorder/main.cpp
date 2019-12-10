#include "device_audios.h"

#include "record_audio_factory.h"
#include "record_desktop_factory.h"

#include "muxer_define.h"
#include "muxer_mp4.h"

#include "error_define.h"
#include "log_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define V_FRAME_RATE 20
#define V_BIT_RATE 64000
#define V_WIDTH 1920
#define V_HEIGHT 1080

#define A_SAMPLE_RATE 44100
#define A_BIT_RATE 128000



static am::record_audio *_recorder_speaker = nullptr;
static am::record_audio *_recorder_microphone = nullptr;
static am::record_desktop *_recorder_desktop = nullptr;

static am::muxer_mp4 *_muxer;

static am::record_audio *audios[2];

int start_muxer() {
	std::string input_id, input_name, out_id, out_name;

	am::device_audios::get_default_input_device(input_id, input_name);

	al_info("use default input aduio device:%s", input_name.c_str());

	am::device_audios::get_default_ouput_device(out_id, out_name);

	al_info("use default output aduio device:%s", out_name.c_str());

	//first audio resrouce must be speaker,otherwise the audio pts may be not correct,may need to change the filter amix descriptions with duration & sync option
	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_speaker);
	
	//to use wasapi ,need to modify the record loop,coz silent will not have data
	//record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_speaker);
	_recorder_speaker->init("audio=virtual-audio-capturer");
	//_recorder_speaker->init("audio=" + out_name);

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_microphone);
	_recorder_microphone->init("audio=" + input_name);

	//record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_GDI, &_recorder_desktop);
	record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_DSHOW, &_recorder_desktop);

	RECORD_DESKTOP_RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = V_WIDTH;
	rect.bottom = V_HEIGHT;

	_recorder_desktop->init(rect, V_FRAME_RATE);

	audios[0] = _recorder_speaker;
	audios[1] = _recorder_microphone;

	_muxer = new am::muxer_mp4();


	am::MUX_SETTING setting;
	setting.v_frame_rate = V_FRAME_RATE;
	setting.v_bit_rate = V_BIT_RATE;
	setting.v_width = V_WIDTH;
	setting.v_height = V_HEIGHT;
	setting.v_qb = 60;

	setting.a_nb_channel = 2;
	setting.a_sample_fmt = AV_SAMPLE_FMT_FLTP;
	setting.a_sample_rate = A_SAMPLE_RATE;
	setting.a_bit_rate = A_BIT_RATE;

	int error = _muxer->init("save.mp4", _recorder_desktop, audios, 2, setting);
	if (error != AE_NO) {
		return error;
	}

	_muxer->start();

	return error;
}

void stop_muxer()
{
	_muxer->stop();

	delete _recorder_desktop;

	if(_recorder_speaker)
		delete _recorder_speaker;

	if (_recorder_microphone)
		delete _recorder_microphone;

	delete _muxer;
}

void test_recorder()
{
	//av_log_set_level(AV_LOG_DEBUG);

	start_muxer();

	getchar();

	//stop have bug that sometime will stuck
	stop_muxer();

	al_info("record stoped...");
	al_info("press any key to exit...");
	system("pause");
}

void show_devices()
{
	std::list<am::DEVICE_AUDIOS> devices;

	am::device_audios::get_input_devices(devices);

	for each (auto device in devices)
	{
		al_info("audio input name:%s id:%s", device.name.c_str(), device.id.c_str());
	}

	am::device_audios::get_output_devices(devices);

	for each (auto device in devices)
	{
		al_info("audio output name:%s id:%s", device.name.c_str(), device.id.c_str());
	}
}

int main(int argc, char **argv)
{
	al_info("record start...");

	show_devices();

	test_recorder();

	return 0;
}
