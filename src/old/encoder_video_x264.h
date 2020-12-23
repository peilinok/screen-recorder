#ifndef ENCODER_VIDEO_X264
#define ENCODER_VIDEO_X264

#include "encoder_video.h"

namespace am {
	class encoder_video_x264 :
		public encoder_video
	{
	public:
		encoder_video_x264();
		~encoder_video_x264();

		int init(int pic_width, int pic_height, int frame_rate, int bit_rate, int qb, int key_pic_sec = 2);

		int get_extradata_size();
		const uint8_t* get_extradata();

		AVCodecID get_codec_id();

	protected:
		void cleanup();
		void encode_loop();

	private:
		int encode(AVFrame *frame, AVPacket *packet);

	private:
		AVCodec *_encoder;
		AVCodecContext *_encoder_ctx;
		AVFrame *_frame;
		uint8_t *_buff;
		int _buff_size;
		int _y_size;
	};
}


#endif
