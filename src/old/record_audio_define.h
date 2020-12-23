#ifndef RECORD_AUDIO_DEFINE
#define RECORD_AUDIO_DEFINE

#include <stdint.h>

namespace am {

/**
* Record type
*
*/
typedef enum RECORD_AUDIO_TYPES {
	AT_AUDIO_NO = 0,
	AT_AUDIO_WAVE,    ///< wave api
	AT_AUDIO_WAS,     ///< wasapi(core audio)
	AT_AUDIO_DSHOW,   ///< direct show api
	AT_AUDIO_FFMPEG,  ///< ffmpeg api
}RECORD_AUDIO_TYPES;

}

#endif // !RECORD_AUDIO_DEFINE
