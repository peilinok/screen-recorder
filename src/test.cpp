
#define AMRECORDER_IMPORT

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib,"Shcore.lib")
#include <ShellScalingApi.h>

#define TEST_NEW

#ifndef TEST_NEW

#include "old\export.h"

void on_preview_image(
	const unsigned char *data,
	unsigned int size,
	int width,
	int height,
	int type) {
	//printf("on_preview_image size:%d type %d\r\n", size, type);
}

int main()
{
	AMRECORDER_DEVICE *speakers = NULL, *mics = NULL;
	AMRECORDER_ENCODERS *vencoders = NULL;

	AMRECORDER_SETTING setting = { 0 };
	AMRECORDER_CALLBACK callback = { 0 };


	int nspeaker = recorder_get_speakers(&speakers);

	int nmic = recorder_get_mics(&mics);

	int n_vencoders = recorder_get_vencoders(&vencoders);

	HDC hdc = GetDC(NULL);

	//when you scale your screen,you shoud aware dpi
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
	setting.v_width = GetSystemMetrics(SM_CXSCREEN);
	setting.v_height = GetSystemMetrics(SM_CYSCREEN);

	setting.v_left = 0;
	setting.v_top = 0;
	setting.v_qb = 100;
	setting.v_bit_rate = 1280 * 1000;
	setting.v_frame_rate = 30;

	DeleteDC(hdc);

	//////////////should be the truely encoder id,zero will always be soft x264
	setting.v_enc_id = 0;

	sprintf(setting.output, "..\\..\\save.mp4");
	//sprintf(setting.output, "..\\..\\save.mkv");

#if 1 //record speaker
	for (int i = 0; i < nspeaker; i++) {
		if (speakers[i].is_default == 1)
			memcpy(&setting.a_speaker, &speakers[i], sizeof(AMRECORDER_DEVICE));
	}
#endif

#if 1 //record mic
	for (int i = 0; i < nmic; i++) {
		if (mics[i].is_default == 1)
			memcpy(&setting.a_mic, &mics[i], sizeof(AMRECORDER_DEVICE));
	}
#endif

	callback.func_preview_yuv = on_preview_image;

	int err = recorder_init(setting, callback);

	err = recorder_start();



	//CAN NOT PAUSE FOR NOW
	/*getchar();

	recorder_pause();

	printf("recorder paused\r\n");

	getchar();

	recorder_resume();

	printf("recorder resumed\r\n");*/

	getchar();


	recorder_stop();

	recorder_release();

	recorder_free_array(speakers);
	recorder_free_array(mics);
	recorder_free_array(vencoders);

	printf("press any key to exit...\r\n");
	system("pause");

	return 0;
}

#else

#include "../include/ray_base.h"
#include "../include/iray_recorder.h"
#include "../include/iray_remuxer.h"
#include "../include/iray_audio_device.h"

#include "../include/ray_autoptr.h"

int main() {
	using namespace ray;
	using namespace recorder;
	using namespace remuxer;

	auto test = []() {

		ray_autoptr<IRecorder> recorder = createRecorder();

		IRecorder::RecorderConfiguration configuration;
		recorder->initialize(configuration);


		uint32_t major, minor, patch, build;

		getVersion(&major, &minor, &patch, &build);

		printf("lib version %d.%d.%d.%d\r\n", major, minor, patch, build);

		ray_autoptr<IRemuxer> remuxer = createRemuxer();

		remuxer->remux("..\\..\\save.mp4", "..\\..\\save.mkv");

		ray_refptr<IAudioDeviceManager> audio_device_mgr = recorder->getAudioDeviceManager();

		ray_refptr<IAudioDeviceCollection> microphone_collection = audio_device_mgr->getMicrophoneCollection();
		if (microphone_collection)
			for (int i = 0; i < microphone_collection->getDeviceCount(); i++) {
				auto device = microphone_collection->getDeviceInfo(i);
				printf("%s   %s\r\n", device.id, device.name);
			}

		ray_refptr<IAudioDeviceCollection> speaker_collection = audio_device_mgr->getSpeakerCollection();
		if (speaker_collection)
			for (int i = 0; i < speaker_collection->getDeviceCount(); i++) {
				auto device = speaker_collection->getDeviceInfo(i);
				printf("%s   %s\r\n", device.id, device.name);
			}
	};

	test();

	getchar();
}

#endif