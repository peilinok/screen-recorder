// Recorder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <exception>

extern "C" {
#include <libavformat\avformat.h>
#include <libavutil\avutil.h>
#include <libavdevice\avdevice.h>
#include "libavcodec\avcodec.h"  
#include "libswscale\swscale.h"
}

int main()
{
	try {
		av_register_all();
		avdevice_register_all();

		AVFormatContext *pFormatCtx = avformat_alloc_context();

		AVInputFormat *ifmt = av_find_input_format("gdigrab");
		avformat_open_input(&pFormatCtx, "desktop", ifmt, NULL);

		if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
			throw std::exception("could not find stream information");
		}

		int videoIndex = -1;
		for (int i = 0; i < pFormatCtx->nb_streams; i++) {
			if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
				videoIndex = i;
				break;
			}
		}

		if (videoIndex == -1)
			throw std::exception("could not find a video stream");

		AVCodecContext *pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
		
		AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

		if (pCodec == NULL)
			throw std::exception("could not find codec");

		if (avcodec_open2(pCodecCtx, pCodec, NULL) != 0) {
			throw std::exception("open codec ffailed");
		}

		AVFrame *pFrameRGB, *pFrameYUV;
		pFrameRGB = av_frame_alloc();
		pFrameYUV = av_frame_alloc();

		uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
		avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

		int ret, gotPic = 0;

		AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));

		struct SwsContext *imgConvertCtx = NULL;
		imgConvertCtx = sws_getContext(
			pCodecCtx->width, 
			pCodecCtx->height, 
			pCodecCtx->pix_fmt, 
			pCodecCtx->width, 
			pCodecCtx->height, 
			AV_PIX_FMT_YUV420P, 
			SWS_BICUBIC, 
			NULL,NULL, NULL);

		for (;;) {
			if (av_read_frame(pFormatCtx, packet) < 0) {
				break;
			}

			if (packet->stream_index == videoIndex) {
				ret = avcodec_decode_video2(pCodecCtx, pFrameRGB, &gotPic, packet);
				if (ret < 0) {
					throw std::exception("decode error");
				}

				if (gotPic) {
					sws_scale(imgConvertCtx, (const uint8_t* const*)pFrameRGB->data, pFrameRGB->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				}

				_sleep(50);
			}

			av_free_packet(packet);
		}//for(;;£©

		av_free(pFrameYUV);
		avcodec_close(pCodecCtx);
		avformat_close_input(&pFormatCtx);
	}
	catch (std::exception ex) {
		printf("%s\r\n", ex.what());
	}

	system("pause");

    return 0;
}

