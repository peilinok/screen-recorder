#ifndef RECORD_AUDIO_DEFINE
#define RECORD_AUDIO_DEFINE

#include <stdint.h>

/**
* Record type
*
*/
typedef enum {
	AT_AUDIO_NO = 0,
	AT_AUDIO_WAVE,    ///< wave api
	AT_AUDIO_WAS,     ///< wasapi(core audio)
	AT_AUDIO_DSHOW,      ///< direct show api
}RECORD_AUDIO_TYPES;

/**
* Some common sample format defines
*
*/
typedef enum {
	AF_AUDIO_NO = 0,

	///packet format,LRLRLRLRLRLR.....
	AF_AUDIO_S16,        ///< signed 16 bits
	AF_AUDIO_S32,        ///< signed 32 bits
	AF_AUDIO_FLT,        ///< float

						 //planar format,LLLL....RRRR....
						 AF_AUDIO_S16P,       ///< signed 16 bits, planar
						 AF_AUDIO_S32P,       ///< signed 32 bits, planar
						 AF_AUDIO_FLTP,       ///< float, planar
}RECORD_AUDIO_FORMAT;

#endif // !RECORD_AUDIO_DEFINE
