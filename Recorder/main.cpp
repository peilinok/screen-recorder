#include "record_audio_factory.h"
#include "resample_pcm.h"
#include "encoder_aac.h"


#include "record_desktop_factory.h"
#include "encoder_264.h"

#include "muxer_define.h"
#include "muxer_mp4.h"
#include "muxer_libmp4v2.h"

#include "error_define.h"
#include "log_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define V_FRAME_RATE 20
#define V_BIT_RATE 64000

#define A_SAMPLE_RATE 48000
#define A_BIT_RATE 128000



static am::record_audio *_recorder_audio = nullptr;
static am::record_audio *_recorder_audio1 = nullptr;
static uint8_t* _sample_frame = NULL;
static int _sample_frame_size = 0;
static int _sample_in_frame = 0;


static am::resample_pcm *_resample_pcm = nullptr;
static uint8_t * _resampled_frame = NULL;
static int _resampled_frame_size = 0;
static am::SAMPLE_SETTING _sample_setting;
static am::SAMPLE_SETTING _resample_setting;

static am::encoder_aac *_encoder_aac = nullptr;

void on_encoder_aac_data(const uint8_t *data, int length)
{
	//al_debug("on aac data:%d", length);
}

void on_encoder_aac_error(int error)
{
	al_fatal("on aac encoder error:%d", error);
}


void on_record_audio_data(AVFrame *frame,int extra_index)
{
	//al_debug("on audio data:%d", length);
}

void on_record_audio_error(int error,int index)
{
	al_error("on audio error:%s\r\n", err2str(error));
}

int start_record_audio() {
	int error = 0;

	error = record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_audio);
	if (error != AE_NO)
		goto exit;

	error = _recorder_audio->init("audio=virtual-audio-capturer");
	if (error != AE_NO)
		goto exit;

	_encoder_aac = new am::encoder_aac();

	_encoder_aac->registe_cb(on_encoder_aac_data, on_encoder_aac_error);

	error = _encoder_aac->init(
		_recorder_audio->get_channel_num(),
		A_SAMPLE_RATE,
		AV_SAMPLE_FMT_FLTP,
		A_BIT_RATE);
	if (error != AE_NO) {
		goto exit;
	}

	al_info(
		"audio sample info: \r\n   \
		 sample_rate: %d\r\n       \
		 bit_rate:%d\r\n           \
	     bit_per_sample:%d\r\n     \
		 channel_num:%d\r\n        \
         format:%d     \r\n        \
		",
		_recorder_audio->get_sample_rate(),
		_recorder_audio->get_bit_rate(),
		_recorder_audio->get_bit_per_sample(),
		_recorder_audio->get_channel_num(),
		_recorder_audio->get_fmt());

	_sample_setting.sample_rate = _recorder_audio->get_sample_rate();
	_sample_setting.nb_samples = _encoder_aac->get_nb_samples();
	_sample_setting.nb_channels = _recorder_audio->get_channel_num();
	_sample_setting.channel_layout = av_get_default_channel_layout(_recorder_audio->get_channel_num());
	_sample_setting.fmt = AV_SAMPLE_FMT_S16;

	_sample_frame_size = av_samples_get_buffer_size(NULL, _sample_setting.nb_channels, _sample_setting.nb_samples, _sample_setting.fmt, 1);
	_sample_frame = (uint8_t*)malloc(_sample_frame_size);

	_resample_setting.sample_rate = A_SAMPLE_RATE;
	_resample_setting.nb_samples = _encoder_aac->get_nb_samples();
	_resample_setting.nb_channels = _recorder_audio->get_channel_num();
	_resample_setting.channel_layout = av_get_default_channel_layout(_resample_setting.nb_channels);
	_resample_setting.fmt = AV_SAMPLE_FMT_FLTP;

	_recorder_audio->registe_cb(on_record_audio_data, on_record_audio_error);

	_resample_pcm = new am::resample_pcm();


	error = _resample_pcm->init(&_sample_setting,&_resample_setting,&_resampled_frame_size);
	if (error == AE_NO && _resampled_frame_size) {
		_resampled_frame = (uint8_t*)malloc(_resampled_frame_size);
	}

	_encoder_aac->start();

	error = _recorder_audio->start();

exit:
	if (error != AE_NO) {
		if (_recorder_audio != nullptr)
			record_audio_destroy(&_recorder_audio);

		_recorder_audio = nullptr;

		if (_resample_pcm != nullptr)
			delete _resampled_frame;

		_resampled_frame = nullptr;

		if (_encoder_aac != nullptr)
			delete _encoder_aac;

		_encoder_aac = nullptr;

		if (_sample_frame)
			free(_sample_frame);

		_sample_frame = NULL;

		if (_resampled_frame)
			free(_sample_frame);

		_sample_frame = NULL;

	}

	return error;
}




static am::encoder_264 *_encoder_264 = nullptr;
static uint8_t *_buff_264 = NULL;
static int _buff_264_len = 0;

void on_encode_264_data(const uint8_t *data, int length,bool key_frame)
{
	static FILE * fp = fopen("a.264", "wb+");
	fwrite(data, 1, length, fp);

	fflush(fp);
}

void on_encode_264_error(int error) {
	al_error("encode error:%d", error);
}

void init_encoder() {
	_encoder_264 = new am::encoder_264();

	int error = _encoder_264->init(1920, 1080, V_FRAME_RATE, V_BIT_RATE, &_buff_264_len);
	if (error != AE_NO)
		goto exit;

	_buff_264 = (uint8_t*)malloc(_buff_264_len);

	_encoder_264->registe_cb(on_encode_264_data, on_encode_264_error);
	_encoder_264->start();

exit:
	if (error != AE_NO) {
		if (_buff_264) {
			free(_buff_264);
		}

		if (_encoder_264)
			delete _encoder_264;

		_encoder_264 = nullptr;
	}
}



static am::record_desktop *_recorder_desktop = nullptr;

void on_record_desktop_data(const uint8_t *data, int data_len) {
	//al_debug("on desktop data:%d \r\n", data_len);
	if (_encoder_264) {
		
#if 1
		_encoder_264->put(data, data_len);
#else
		int got_pic = 0;
		int err = _encoder_264->encode(data, data_len, _buff_264, _buff_264_len, &got_pic);
		if (err == AE_NO && got_pic) {
			//al_debug("got 264 data:%d \r\n", got_pic);
			static FILE * fp = fopen("a.264", "wb+");
			fwrite(_buff_264, 1, got_pic, fp);
		}
#endif
	}
}

void on_record_desktop_error(int error) {
	al_error("on video error:%s\r\n", err2str(error));
}

int start_record_desktop() {
	int error = AE_NO;
	
	error = record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_GDI, &_recorder_desktop);
	if (error != AE_NO)
		goto exit;

	RECORD_DESKTOP_RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = 1920;
	rect.bottom = 1080;
	error = _recorder_desktop->init(rect, V_FRAME_RATE);

	_recorder_desktop->registe_cb(on_record_desktop_data, on_record_desktop_error);

	init_encoder();

	error = _recorder_desktop->start();

exit:
	if (error != AE_NO) {
		if (_recorder_desktop != nullptr)
			record_desktop_destroy(&_recorder_desktop);
	}
	return error;
}

#define MUXE_MP4


#ifdef MUXE_MP4
static am::muxer_mp4 *_muxer;
#else
static am::muxer_libmp4v2 *_muxer;
#endif
static am::record_audio *audios[2];

int start_muxer() {
	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_audio);
	_recorder_audio->init("audio=virtual-audio-capturer");

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_audio1);
	//_recorder_audio1->init("audio=Âó¿Ë·ç (Realtek(R) Audio)");


	record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_GDI, &_recorder_desktop);
	RECORD_DESKTOP_RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = 1920;
	rect.bottom = 1080;

	_recorder_desktop->init(rect, V_FRAME_RATE);

	audios[0] = _recorder_audio;

#ifdef MUXE_MP4
	_muxer = new am::muxer_mp4();
#else 
	_muxer = new am::muxer_libmp4v2();
#endif

	am::MUX_SETTING setting;
	setting.v_frame_rate = V_FRAME_RATE;
	setting.v_bit_rate = V_BIT_RATE;

	setting.a_nb_channel = 2;
	setting.a_sample_fmt = AV_SAMPLE_FMT_FLTP;
	setting.a_sample_rate = A_SAMPLE_RATE;
	setting.a_bit_rate = A_BIT_RATE;

	int error = _muxer->init("save.mp4", _recorder_desktop, audios, 1, setting);
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

	if(_recorder_audio)
		delete _recorder_audio;

	if (_recorder_audio1)
		delete _recorder_audio1;

	delete _muxer;
}



#include <portaudio.h>

#define PA_USE_ASIO 0

#if PA_USE_ASIO
#include "pa_asio.h"
#endif

static void PrintSupportedStandardSampleRates(
	const PaStreamParameters *inputParameters,
	const PaStreamParameters *outputParameters)
{
	static double standardSampleRates[] = {
		8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
		44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1 /* negative terminated  list */
	};
	int     i, printCount;
	PaError err;

	printCount = 0;
	for (i = 0; standardSampleRates[i] > 0; i++)
	{
		err = Pa_IsFormatSupported(inputParameters, outputParameters, standardSampleRates[i]);
		if (err == paFormatIsSupported)
		{
			if (printCount == 0)
			{
				printf("\t%8.2f", standardSampleRates[i]);
				printCount = 1;
			}
			else if (printCount == 4)
			{
				printf(",\n\t%8.2f", standardSampleRates[i]);
				printCount = 1;
			}
			else
			{
				printf(", %8.2f", standardSampleRates[i]);
				++printCount;
			}
		}
	}
	if (!printCount)
		printf("None\n");
	else
		printf("\n");
}

int enum_audio_devices() {
	int     i, numDevices, defaultDisplayed;
	const   PaDeviceInfo *deviceInfo;
	PaStreamParameters inputParameters, outputParameters;
	PaError err;


	err = Pa_Initialize();
	if (err != paNoError)
	{
		printf("ERROR: Pa_Initialize returned 0x%x\n", err);
		goto error;
	}

	printf("PortAudio version: 0x%08X\n", Pa_GetVersion());
	printf("Version text: '%s'\n", Pa_GetVersionInfo()->versionText);

	numDevices = Pa_GetDeviceCount();
	if (numDevices < 0)
	{
		printf("ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices);
		err = numDevices;
		goto error;
	}

	printf("Number of devices = %d\n", numDevices);
	for (i = 0; i<numDevices; i++)
	{
		deviceInfo = Pa_GetDeviceInfo(i);
		printf("--------------------------------------- device #%d\n", i);

		/* Mark global and API specific default devices */
		defaultDisplayed = 0;
		if (i == Pa_GetDefaultInputDevice())
		{
			printf("[ Default Input");
			defaultDisplayed = 1;
		}
		else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice)
		{
			const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
			printf("[ Default %s Input", hostInfo->name);
			defaultDisplayed = 1;
		}

		if (i == Pa_GetDefaultOutputDevice())
		{
			printf((defaultDisplayed ? "," : "["));
			printf(" Default Output");
			defaultDisplayed = 1;
		}
		else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice)
		{
			const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
			printf((defaultDisplayed ? "," : "["));
			printf(" Default %s Output", hostInfo->name);
			defaultDisplayed = 1;
		}

		if (defaultDisplayed)
			printf(" ]\n");

		/* print device info fields */
#ifdef WIN32
		{   /* Use wide char on windows, so we can show UTF-8 encoded device names */
			wchar_t wideName[MAX_PATH];
			MultiByteToWideChar(CP_UTF8, 0, deviceInfo->name, -1, wideName, MAX_PATH - 1);
			wprintf(L"Name                        = %s\n", wideName);
		}
#else
		printf("Name                        = %s\n", deviceInfo->name);
#endif
		printf("Host API                    = %s\n", Pa_GetHostApiInfo(deviceInfo->hostApi)->name);
		printf("Max inputs = %d", deviceInfo->maxInputChannels);
		printf(", Max outputs = %d\n", deviceInfo->maxOutputChannels);

		printf("Default low input latency   = %8.4f\n", deviceInfo->defaultLowInputLatency);
		printf("Default low output latency  = %8.4f\n", deviceInfo->defaultLowOutputLatency);
		printf("Default high input latency  = %8.4f\n", deviceInfo->defaultHighInputLatency);
		printf("Default high output latency = %8.4f\n", deviceInfo->defaultHighOutputLatency);

#ifdef WIN32
#if PA_USE_ASIO
		/* ASIO specific latency information */
		if (Pa_GetHostApiInfo(deviceInfo->hostApi)->type == paASIO) {
			long minLatency, maxLatency, preferredLatency, granularity;

			err = PaAsio_GetAvailableLatencyValues(i,
				&minLatency, &maxLatency, &preferredLatency, &granularity);

			printf("ASIO minimum buffer size    = %ld\n", minLatency);
			printf("ASIO maximum buffer size    = %ld\n", maxLatency);
			printf("ASIO preferred buffer size  = %ld\n", preferredLatency);

			if (granularity == -1)
				printf("ASIO buffer granularity     = power of 2\n");
			else
				printf("ASIO buffer granularity     = %ld\n", granularity);
		}
#endif /* PA_USE_ASIO */
#endif /* WIN32 */

		printf("Default sample rate         = %8.2f\n", deviceInfo->defaultSampleRate);

		/* poll for standard sample rates */
		inputParameters.device = i;
		inputParameters.channelCount = deviceInfo->maxInputChannels;
		inputParameters.sampleFormat = paInt16;
		inputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
		inputParameters.hostApiSpecificStreamInfo = NULL;

		outputParameters.device = i;
		outputParameters.channelCount = deviceInfo->maxOutputChannels;
		outputParameters.sampleFormat = paInt16;
		outputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
		outputParameters.hostApiSpecificStreamInfo = NULL;

		if (inputParameters.channelCount > 0)
		{
			printf("Supported standard sample rates\n for half-duplex 16 bit %d channel input = \n",
				inputParameters.channelCount);
			PrintSupportedStandardSampleRates(&inputParameters, NULL);
		}

		if (outputParameters.channelCount > 0)
		{
			printf("Supported standard sample rates\n for half-duplex 16 bit %d channel output = \n",
				outputParameters.channelCount);
			PrintSupportedStandardSampleRates(NULL, &outputParameters);
		}

		if (inputParameters.channelCount > 0 && outputParameters.channelCount > 0)
		{
			printf("Supported standard sample rates\n for full-duplex 16 bit %d channel input, %d channel output = \n",
				inputParameters.channelCount, outputParameters.channelCount);
			PrintSupportedStandardSampleRates(&inputParameters, &outputParameters);
		}
	}

	Pa_Terminate();

	printf("----------------------------------------------\n");
	return 0;

error:
	Pa_Terminate();
	fprintf(stderr, "Error number: %d\n", err);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
	return err;
}






int main(int argc, char **argv)
{
	al_info("record start...");
	al_debug("record param is :");

	for (int i = 1; i < argc; i++) {
		al_debug("%s",argv[i]);
	}

	//enum_audio_devices();

	//start_record_audio();
	//start_record_desktop();
	start_muxer();

	getchar();

	stop_muxer();

	system("pause");
}
