#include "record_audio_factory.h"
#include "record_desktop.h"

#include "encoder_aac.h"
#include "encoder_aac.h"

#include "muxer_mp4.h"

#include "error_define.h"
#include "log_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

static am::record_audio *_recorder_audio = nullptr;

void on_record_audio_data(const uint8_t *data, int length)
{
	al_debug("on audio data:%d\r\n", length);
}

void on_record_audio_error(int error)
{
	al_error("on audio error:%s\r\n", err2str(error));
}

int start_record_audio() {
	int error = 0;

	error = record_audio_new(RECORD_AUDIO_TYPES::AT_AUDIO_WAS, &_recorder_audio);
	if (error != AE_NO)
		goto exit;

	error = _recorder_audio->init("", RECORD_AUDIO_FORMAT::AF_AUDIO_FLT, 48000, 3072000);
	if (error != AE_NO)
		goto exit;

	_recorder_audio->registe_cb(on_record_audio_data, on_record_audio_error);

	error = _recorder_audio->start();

exit:
	if (error != AE_NO) {
		if (_recorder_audio != nullptr)
			record_audio_destroy(&_recorder_audio);
	}

	return error;
}

int start_record_desktop() {
	int error = AE_NO;

	return error;
}



int main(int argc, char **argv)
{
	al_info("record start...");
	al_debug("record param is :");

	for (int i = 1; i < argc; i++) {
		al_debug("%s",argv[i]);
	}

	start_record_audio();

	do {
		Sleep(20);
	} while (1);
}
