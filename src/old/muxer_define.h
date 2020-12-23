#ifndef MUXER_DEFINE
#define MUXER_DEFINE

#include "encoder_video_define.h"

namespace am {
	typedef struct {
		uint8_t *buff;
		int size;
		int sample_in;
	}AUDIO_SAMPLE;

	class encoder_video;
	class record_desktop;
	class sws_helper;

	class encoder_aac;
	class filter_amix;
	class filter_aresample;
	class record_audio;

	typedef struct MUX_SETTING_T {
		int v_frame_rate;
		int v_bit_rate;
		int v_width;
		int v_height;
		int v_out_width;
		int v_out_height;
		int v_qb;
		ENCODER_VIDEO_ID v_encoder_id;

		int a_nb_channel;
		int a_sample_rate;
		AVSampleFormat a_sample_fmt;
		int a_bit_rate;
	}MUX_SETTING;

	typedef struct MUX_STREAM_T {
		//common
		AVStream *st;               // av stream
		AVBitStreamFilterContext *filter; //pps|sps adt

		uint64_t pre_pts;

		MUX_SETTING setting;        // output setting

		//video
		encoder_video *v_enc;         // video encoder
		record_desktop *v_src;      // video source
		sws_helper *v_sws;          // video sws

		//audio
		encoder_aac *a_enc;                     // audio encoder
		filter_amix *a_filter_amix;             // audio mixer
		filter_aresample **a_filter_aresample;  // audio resamplers
		int a_nb;							    // audio source num
		record_audio **a_src;				    // audio sources
		AUDIO_SAMPLE **a_samples;			    // audio sample data
		AUDIO_SAMPLE **a_resamples;			    // audio resampled data
	}MUX_STREAM;
}

#endif