#include "device_audios.h"

#include "record_audio_factory.h"
#include "record_desktop_factory.h"

#include "encoder_aac.h"
#include "resample_pcm.h"
#include "filter_aresample.h"

#include "muxer_define.h"
#include "muxer_mp4.h"

#include "utils_string.h"
#include "utils_winversion.h"
#include "error_define.h"
#include "log_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define USE_WASAPI 1

#define V_FRAME_RATE 25
#define V_BIT_RATE 64000
#define V_WIDTH 1920
#define V_HEIGHT 1080
#define V_QUALITY 80

#define A_SAMPLE_CHANNEL 2
#define A_SAMPLE_RATE 44100
#define A_BIT_RATE 128000


//for test muxer
static am::record_audio *_recorder_speaker = nullptr;
static am::record_audio *_recorder_microphone = nullptr;
static am::record_desktop *_recorder_desktop = nullptr;
static am::muxer_mp4 *_muxer;
static am::record_audio *audios[2];

//for test audio record
static am::record_audio *_recorder_audio = nullptr;
static am::encoder_aac *_encoder_aac = nullptr;
static am::resample_pcm *_resample_pcm = nullptr;
static am::filter_aresample *_filter_aresample = nullptr;

static int _sample_in = 0;
static int _sample_size = 0;
static int _resample_size = 0;
static uint8_t *_sample_buffer = nullptr;
static uint8_t *_resample_buffer = nullptr;

int start_muxer() {
	std::string input_id, input_name, out_id, out_name;

	am::device_audios::get_default_input_device(input_id, input_name);

	al_info("use default input aduio device:%s", input_name.c_str());

	am::device_audios::get_default_ouput_device(out_id, out_name);

	al_info("use default output aduio device:%s", out_name.c_str());

	//first audio resrouce must be speaker,otherwise the audio pts may be not correct,may need to change the filter amix descriptions with duration & sync option
#if !USE_WASAPI

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_speaker);
	_recorder_speaker->init(
		am::utils_string::ascii_utf8("audio=virtual-audio-capturer"),
		am::utils_string::ascii_utf8("audio=virtual-audio-capturer"), 
		false
	);

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_microphone);
	_recorder_microphone->init(am::utils_string::ascii_utf8("audio=") + input_name, input_id, true);
#else
	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_speaker);
	_recorder_speaker->init(out_name, out_id, false);

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_microphone);
	_recorder_microphone->init(input_name, input_id, true);
#endif // !USE_WASAPI


	record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_WIN_DUPLICATION, &_recorder_desktop);
	//record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_WIN_GDI, &_recorder_desktop);
	//record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_FFMPEG_DSHOW, &_recorder_desktop);

	RECORD_DESKTOP_RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = V_WIDTH;
	rect.bottom = V_HEIGHT;

	_recorder_desktop->init(rect, V_FRAME_RATE);

	audios[0] = _recorder_microphone;
	audios[1] = _recorder_speaker;

	_muxer = new am::muxer_mp4();


	am::MUX_SETTING setting;
	setting.v_frame_rate = V_FRAME_RATE;
	setting.v_bit_rate = V_BIT_RATE;
	setting.v_width = V_WIDTH;
	setting.v_height = V_HEIGHT;
	setting.v_qb = V_QUALITY;

	setting.a_nb_channel = A_SAMPLE_CHANNEL;
	setting.a_sample_fmt = AV_SAMPLE_FMT_FLTP;
	setting.a_sample_rate = A_SAMPLE_RATE;
	setting.a_bit_rate = A_BIT_RATE;

	int error = _muxer->init(am::utils_string::ascii_utf8("..\\..\\save.mp4").c_str(), _recorder_desktop, audios, 2, setting);
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

void on_aac_data(AVPacket *packet) {
	al_debug("on aac data :%d", packet->size);
}

void on_aac_error(int) {

}

void on_pcm_error(int, int) {

}

void on_pcm_data1(AVFrame *frame, int index) {

	int copied_len = 0;
	int sample_len = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
	int remain_len = sample_len;

	int is_planner = av_sample_fmt_is_planar((AVSampleFormat)frame->format);

	while (remain_len > 0) {//should add is_planner codes
							//cache pcm
		copied_len = min(_sample_size - _sample_in, remain_len);
		if (copied_len) {
			memcpy(_sample_buffer + _sample_in, frame->data[0] + sample_len - remain_len, copied_len);
			_sample_in += copied_len;
			remain_len = remain_len - copied_len;
		}

		//got enough pcm to encoder,resample and mix
		if (_sample_in == _sample_size) {
			int ret = _resample_pcm->convert(_sample_buffer, _sample_size, _resample_buffer,_resample_size);
			if (ret > 0) {
				_encoder_aac->put(_resample_buffer, _resample_size, frame);
			}
			else {
				al_debug("resample audio %d failed,%d", index, ret);
			}

			_sample_in = 0;
		}
	}
}

void on_pcm_data(AVFrame *frame, int index) {
	_filter_aresample->add_frame(frame);
}

void on_aresample_data(AVFrame * frame,int index) {
	int copied_len = 0;
	int sample_len = av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
	sample_len = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);

	int remain_len = sample_len;

	//for data is planar,should copy data[0] data[1] to correct buff pos
	if (av_sample_fmt_is_planar((AVSampleFormat)frame->format) == 0) {
		while (remain_len > 0) {
			//cache pcm
			copied_len = min(_sample_size - _sample_in, remain_len);
			if (copied_len) {
				memcpy(_resample_buffer + _sample_in, frame->data[0] + sample_len - remain_len, copied_len);
				_sample_in += copied_len;
				remain_len = remain_len - copied_len;
			}

			//got enough pcm to encoder,resample and mix
			if (_sample_in == _sample_size) {
				_encoder_aac->put(_resample_buffer, _sample_size, frame);

				_sample_in = 0;
			}
		}
	}
	else {//resample size is channels*frame->linesize[0],for 2 channels
		while (remain_len > 0) {
			copied_len = min(_sample_size - _sample_in, remain_len);
			if (copied_len) {
				memcpy(_resample_buffer + _sample_in / 2, frame->data[0] + (sample_len - remain_len) / 2, copied_len / 2);
				memcpy(_resample_buffer + _sample_size / 2 + _sample_in / 2, frame->data[1] + (sample_len - remain_len) / 2, copied_len / 2);
				_sample_in += copied_len;
				remain_len = remain_len - copied_len;
			}

			if (_sample_in == _sample_size) {
				_encoder_aac->put(_resample_buffer, _sample_size, frame);

				_sample_in = 0;
			}
		}
	}
}

void on_aresample_error(int error,int index) {

}

void save_aac() {
	std::string input_id, input_name, out_id, out_name;

	am::device_audios::get_default_input_device(input_id, input_name);

	am::device_audios::get_default_ouput_device(out_id, out_name);

#if 1
	al_info("use default \r\noutput aduio device name:%s \r\noutput audio device id:%s ",
		out_name.c_str(), out_id.c_str());

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_audio);
	_recorder_audio->init(out_name, out_id, false);
#else
	al_info("use default \r\ninput aduio device name:%s \r\ninput audio device id:%s ",
		input_name.c_str(), input_id.c_str());

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_audio);
	_recorder_audio->init(input_name,input_id, true);
#endif

	_recorder_audio->registe_cb(on_pcm_data, on_pcm_error, 0);

	_encoder_aac = new am::encoder_aac();
	_encoder_aac->init(A_SAMPLE_CHANNEL, A_SAMPLE_RATE, AV_SAMPLE_FMT_FLTP, A_BIT_RATE);
	//_encoder_aac->registe_cb(on_aac_data, on_aac_error);

	am::SAMPLE_SETTING src, dst = { 0 };
	src = {
		_encoder_aac->get_nb_samples(),
		av_get_default_channel_layout(_recorder_audio->get_channel_num()),
		_recorder_audio->get_channel_num(),
		_recorder_audio->get_fmt(),
		_recorder_audio->get_sample_rate()
	};
	dst = {
		_encoder_aac->get_nb_samples(),
		av_get_default_channel_layout(A_SAMPLE_CHANNEL),
		A_SAMPLE_CHANNEL,
		AV_SAMPLE_FMT_FLTP,
		A_SAMPLE_RATE
	};

	_resample_pcm = new am::resample_pcm();
	_resample_pcm->init(&src, &dst, &_resample_size);
	_resample_buffer = new uint8_t[_resample_size];

	_sample_size = av_samples_get_buffer_size(NULL, A_SAMPLE_CHANNEL, _encoder_aac->get_nb_samples(), _recorder_audio->get_fmt(), 1);
	_sample_buffer = new uint8_t[_sample_size];

	_filter_aresample = new am::filter_aresample();
	_filter_aresample->init({
		NULL,NULL,
		_recorder_audio->get_time_base(),
		_recorder_audio->get_sample_rate(),
		_recorder_audio->get_fmt(),
		_recorder_audio->get_channel_num(),
		av_get_default_channel_layout(_recorder_audio->get_channel_num())
	}, {
		NULL,NULL,
		{ 1,AV_TIME_BASE },
		A_SAMPLE_RATE,
		AV_SAMPLE_FMT_FLTP,
		A_SAMPLE_CHANNEL,
		av_get_default_channel_layout(A_SAMPLE_CHANNEL)
	},0);
	_filter_aresample->registe_cb(on_aresample_data, on_aresample_error);


	_filter_aresample->start();
	_encoder_aac->start();
	_recorder_audio->start();

	getchar();

	_recorder_audio->stop();
	_filter_aresample->stop();

	delete _recorder_audio;
	delete _encoder_aac;
	delete _resample_pcm;
	delete _filter_aresample;

	delete[] _sample_buffer;
	delete[] _resample_buffer;
}

void test_audio() 
{
	std::string input_id, input_name, out_id, out_name;

	am::device_audios::get_default_input_device(input_id, input_name);

	al_info("use default \r\ninput aduio device name:%s \r\ninput audio device id:%s ", 
		input_name.c_str(), input_id.c_str());

	am::device_audios::get_default_ouput_device(out_id, out_name);

	al_info("use default \r\noutput aduio device name:%s \r\noutput audio device id:%s ", 
		out_name.c_str(), out_id.c_str());

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_speaker);
	_recorder_speaker->init(am::utils_string::ascii_utf8("Default"), am::utils_string::ascii_utf8("Default"), false);


	//record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_microphone);
	//_recorder_microphone->init(input_name,input_id, true);

	_recorder_speaker->start();

	//_recorder_microphone->start();

	getchar();

	_recorder_speaker->stop();
	//_recorder_microphone->stop();
}

int main(int argc, char **argv)
{
	al_info("record start...");

	am::winversion_info ver = { 0 };

	am::utils_winversion::get_win(&ver);

	bool is_win8_or_above = am::utils_winversion::is_win8_or_above();

	bool is_ia32 = am::utils_winversion::is_32();

	al_info("win version: %d.%d.%d.%d", ver.major, ver.minor, ver.build, ver.revis);
	al_info("is win8 or above: %s", is_win8_or_above ? "true" : "false");

	show_devices();

	//test_audio();

	test_recorder();

	//save_aac();

	al_info("press any key to exit...");
	system("pause");

	return 0;
}
