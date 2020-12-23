#ifndef RECORD_AUDIO_FACTORY
#define RECORD_AUDIO_FACTORY

#include "record_audio.h"

/**
*  Create a new audio record context
*
*/
int record_audio_new(am::RECORD_AUDIO_TYPES type, am::record_audio **recorder);

/**
* Destroy audio record context
*
*/
void record_audio_destroy(am::record_audio ** recorder);

#endif // !RECORD_AUDIO_FACTORY

