#pragma once

namespace am {
	typedef struct {

	}EXTRA_DATA;

	typedef struct {
		uint8_t *buff;
		int size;
		int sample_in;
	}AUDIO_SAMPLE;

	class encoder_264;
	class record_desktop;
	class encoder_aac;
	class filter_amix;
	class record_audio;
	class resample_pcm;
	class ring_buffer;

	typedef struct MUX_STREAM_T {
		//common
		AVStream *st;               // av stream
		int64_t cur_pts;            // pts of the current frame that will be generated
		int64_t cur_frame_index;    // current frame index
		AVBitStreamFilterContext *filter; //pps|sps adt

		AVFrame *frame;             // current av frame
		AVFrame *tmp_frame;         // tmp av frame 

		ring_buffer *buffer;

		//video
		encoder_264 *v_enc;         // video encoder
		record_desktop *v_src;      // video source

									//audio
		encoder_aac *a_enc;         // audio encoder
		filter_amix *a_filter;      // audio mixer
		int a_nb;                   // audio source num
		record_audio **a_src;       // audio sources
		resample_pcm **a_rs;        // audio resamplers
		AUDIO_SAMPLE **a_samples;   // audio sample data
		AUDIO_SAMPLE **a_resamples; // audio resampled data
	}MUX_STREAM;

	typedef struct MUX_SETTING_T{
		int v_frame_rate;
		int v_bit_rate;

		int a_nb_samples;
		int a_nb_channel;
		int a_sample_rate;
		AVSampleFormat a_sample_fmt;
		int a_bit_rate;
	}MUX_SETTING;
}