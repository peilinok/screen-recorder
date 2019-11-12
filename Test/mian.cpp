
#define AMRECORDER_IMPORT
#include "../Recorder/export.h"

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
	AMRECORDER_DEVICE *speakers = NULL, *mics = NULL;

	AMRECORDER_SETTING setting;
	AMRECORDER_CALLBACK callback;

	int nspeaker = recorder_get_speakers(&speakers);
	
	int nmic = recorder_get_mics(&mics);

	setting.v_left = 0;
	setting.v_top = 0;
	setting.v_width = 1920;
	setting.v_height = 1080;
	setting.v_qb = 60;
	setting.v_bit_rate = 64000;
	setting.v_frame_rate = 20;

	sprintf(setting.output, "save.mp4");

	for (int i = 0; i < nspeaker; i++) {
		if (speakers[i].is_default == 1)
			memcpy(&setting.a_speaker, &speakers[i], sizeof(AMRECORDER_DEVICE));
	}

	for (int i = 0; i < nmic; i++) {
		if (mics[i].is_default == 1)
			memcpy(&setting.a_mic, &mics[i], sizeof(AMRECORDER_DEVICE));
	}

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

	delete[] speakers;
	delete[] mics;

	printf("press any key to exit...\r\n");
	system("pause");

	return 0;
}