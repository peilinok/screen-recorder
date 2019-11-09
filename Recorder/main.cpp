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
#define V_WIDTH 1920
#define V_HEIGHT 1080

#define A_SAMPLE_RATE 44100
#define A_BIT_RATE 128000



static am::record_audio *_recorder_audio = nullptr;
static am::record_audio *_recorder_audio1 = nullptr;
static am::encoder_aac *_encoder_aac = nullptr;

static am::encoder_264 *_encoder_264 = nullptr;
static am::record_desktop *_recorder_desktop = nullptr;

static am::muxer_mp4 *_muxer;

static am::record_audio *audios[2];

int start_muxer() {
	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_audio);
	_recorder_audio->init("audio=virtual-audio-capturer");

	record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_DSHOW, &_recorder_audio1);
	_recorder_audio1->init("audio=Âó¿Ë·ç (Realtek(R) Audio)");

	//record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_GDI, &_recorder_desktop);
	record_desktop_new(RECORD_DESKTOP_TYPES::DT_DESKTOP_DSHOW, &_recorder_desktop);

	RECORD_DESKTOP_RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = V_WIDTH;
	rect.bottom = V_HEIGHT;

	_recorder_desktop->init(rect, V_FRAME_RATE);

	audios[0] = _recorder_audio;
	audios[1] = _recorder_audio1;

	_muxer = new am::muxer_mp4();


	am::MUX_SETTING setting;
	setting.v_frame_rate = V_FRAME_RATE;
	setting.v_bit_rate = V_BIT_RATE;
	setting.v_width = V_WIDTH;
	setting.v_height = V_HEIGHT;

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

	//enum_audio_devices();

	start_muxer();

	getchar();

	stop_muxer();

	al_info("record stoped...");
	al_info("press any key to exit...");
	system("pause");

	return 0;
}
