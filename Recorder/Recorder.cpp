// Recorder.cpp : Defines the entry point for the console application.
//

#include <exception>
#include <iostream>

extern "C" {
#include "common.h"
}


void capture_screen()
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

		AVCodecContext *pDecoderCtx = pFormatCtx->streams[videoIndex]->codec;

		//init decoder
		AVCodec *pDecoder = avcodec_find_decoder(pDecoderCtx->codec_id);

		if (pDecoder == NULL)
			throw std::exception("could not find codec");

		if (avcodec_open2(pDecoderCtx, pDecoder, NULL) != 0) {
			throw std::exception("open codec ffailed");
		}

		//init transfer and buffer
		AVFrame *pFrameRGB, *pFrameYUV;
		pFrameRGB = av_frame_alloc();
		pFrameYUV = av_frame_alloc();

		uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pDecoderCtx->width, pDecoderCtx->height));
		avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pDecoderCtx->width, pDecoderCtx->height);

		int ret, gotPic = 0;

		AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));

		struct SwsContext *imgConvertCtx = NULL;
		imgConvertCtx = sws_getContext(
			pDecoderCtx->width,
			pDecoderCtx->height,
			pDecoderCtx->pix_fmt,
			pDecoderCtx->width,
			pDecoderCtx->height,
			AV_PIX_FMT_YUV420P,
			SWS_BICUBIC,
			NULL, NULL, NULL);

		//init encoder
		AVCodec *pEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (pEncoder == NULL)
			throw std::exception("could not find encoder");

		AVCodecContext *pEncoderCtx = avcodec_alloc_context3(pEncoder);
		if (pEncoderCtx == NULL)
			throw std::exception("could not alloc context for encoder");

		pEncoderCtx->codec_id = AV_CODEC_ID_H264;
		pEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
		pEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		pEncoderCtx->width = pDecoderCtx->width;
		pEncoderCtx->height = pDecoderCtx->height;
		pEncoderCtx->time_base.num = 1;
		pEncoderCtx->time_base.den = 20;//֡��(��һ���Ӷ�����ͼƬ)
		pEncoderCtx->bit_rate = 4000000; //������(���������С���Ըı�������Ƶ������)
		pEncoderCtx->gop_size = 12;

		if (pEncoderCtx->flags & AVFMT_GLOBALHEADER)
			pEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		AVDictionary *param = 0;

		av_dict_set(&param, "preset", "superfast", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);

		pEncoder = avcodec_find_encoder(pEncoderCtx->codec_id);
		if (pEncoder == NULL)
			throw std::exception("could not find encoder by context");

		if (avcodec_open2(pEncoderCtx, pEncoder, &param) != 0)
			throw std::exception("open encoder failed");

		AVFrame *pFrame264 = av_frame_alloc();
		int frame264Size = avpicture_get_size(pEncoderCtx->pix_fmt, pEncoderCtx->width, pEncoderCtx->height);
		uint8_t *frame264Buf = (uint8_t*)av_malloc(frame264Size);
		avpicture_fill((AVPicture*)pFrame264, frame264Buf, pEncoderCtx->pix_fmt, pEncoderCtx->width, pEncoderCtx->height);

		AVPacket pkt264;
		av_new_packet(&pkt264, frame264Size);

		int Y_size = pEncoderCtx->width * pEncoderCtx->height;

		for (;;) {
			if (av_read_frame(pFormatCtx, packet) < 0) {
				break;
			}

			if (packet->stream_index == videoIndex) {
				ret = avcodec_decode_video2(pDecoderCtx, pFrameRGB, &gotPic, packet);
				if (ret < 0) {
					throw std::exception("decode error");
				}

				//trans rgb to yuv420 in out_buffer
				if (gotPic) {
					sws_scale(imgConvertCtx, (const uint8_t* const*)pFrameRGB->data, pFrameRGB->linesize, 0, pDecoderCtx->height, pFrameYUV->data, pFrameYUV->linesize);

					gotPic = 0;
					pFrame264->data[0] = out_buffer;//Y
					pFrame264->data[1] = out_buffer + Y_size;//U
					pFrame264->data[2] = out_buffer + Y_size * 5 / 4;//V

					ret = avcodec_encode_video2(pEncoderCtx, &pkt264, pFrame264, &gotPic);
					if (gotPic == 1) {
						printf("264 packet:%d\r\n", pkt264.size);

						static FILE *fp = fopen("a.264", "wb+");

						fwrite(pkt264.data, 1, pkt264.size, fp);

						av_free_packet(&pkt264);
					}
				}

				_sleep(50);
			}

			av_free_packet(packet);
		}//for(;;��

		av_free(pFrameYUV);
		avcodec_close(pDecoderCtx);
		avformat_close_input(&pFormatCtx);
	}
	catch (std::exception ex) {
		printf("%s\r\n", ex.what());
	}
}


#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

#define MoveMemory RtlMoveMemory
#define CopyMemory RtlCopyMemory
#define FillMemory RtlFillMemory
#define ZeroMemory RtlZeroMemory

#define min(a,b)            (((a) < (b)) ? (a) : (b))

//
//  WAV file writer.
//
//  This is a VERY simple .WAV file writer.
//

//
//  A wave file consists of:
//
//  RIFF header:    8 bytes consisting of the signature "RIFF" followed by a 4 byte file length.
//  WAVE header:    4 bytes consisting of the signature "WAVE".
//  fmt header:     4 bytes consisting of the signature "fmt " followed by a WAVEFORMATEX 
//  WAVEFORMAT:     <n> bytes containing a waveformat structure.
//  DATA header:    8 bytes consisting of the signature "data" followed by a 4 byte file length.
//  wave data:      <m> bytes containing wave data.
//
//
//  Header for a WAV file - we define a structure describing the first few fields in the header for convenience.
//
struct WAVEHEADER
{
	DWORD   dwRiff;                     // "RIFF"
	DWORD   dwSize;                     // Size
	DWORD   dwWave;                     // "WAVE"
	DWORD   dwFmt;                      // "fmt "
	DWORD   dwFmtSize;                  // Wave Format Size
};

//  Static RIFF header, we'll append the format to it.
const BYTE WaveHeader[] =
{
	'R',   'I',   'F',   'F',  0x00,  0x00,  0x00,  0x00, 'W',   'A',   'V',   'E',   'f',   'm',   't',   ' ', 0x00, 0x00, 0x00, 0x00
};

//  Static wave DATA tag.
const BYTE WaveData[] = { 'd', 'a', 't', 'a' };

//
//  Write the contents of a WAV file.  We take as input the data to write and the format of that data.
//
bool WriteWaveFile(HANDLE FileHandle, const BYTE *Buffer, const size_t BufferSize, const WAVEFORMATEX *WaveFormat)
{
	DWORD waveFileSize = sizeof(WAVEHEADER) + sizeof(WAVEFORMATEX) + WaveFormat->cbSize + sizeof(WaveData) + sizeof(DWORD) + static_cast<DWORD>(BufferSize);
	BYTE *waveFileData = new (std::nothrow) BYTE[waveFileSize];
	BYTE *waveFilePointer = waveFileData;
	WAVEHEADER *waveHeader = reinterpret_cast<WAVEHEADER *>(waveFileData);

	if (waveFileData == NULL)
	{
		printf("Unable to allocate %d bytes to hold output wave data\n", waveFileSize);
		return false;
	}

	//
	//  Copy in the wave header - we'll fix up the lengths later.
	//
	CopyMemory(waveFilePointer, WaveHeader, sizeof(WaveHeader));
	waveFilePointer += sizeof(WaveHeader);

	//
	//  Update the sizes in the header.
	//
	waveHeader->dwSize = waveFileSize - (2 * sizeof(DWORD));
	waveHeader->dwFmtSize = sizeof(WAVEFORMATEX) + WaveFormat->cbSize;

	//
	//  Next copy in the WaveFormatex structure.
	//
	CopyMemory(waveFilePointer, WaveFormat, sizeof(WAVEFORMATEX) + WaveFormat->cbSize);
	waveFilePointer += sizeof(WAVEFORMATEX) + WaveFormat->cbSize;


	//
	//  Then the data header.
	//
	CopyMemory(waveFilePointer, WaveData, sizeof(WaveData));
	waveFilePointer += sizeof(WaveData);
	*(reinterpret_cast<DWORD *>(waveFilePointer)) = static_cast<DWORD>(BufferSize);
	waveFilePointer += sizeof(DWORD);

	//
	//  And finally copy in the audio data.
	//
	CopyMemory(waveFilePointer, Buffer, BufferSize);

	//
	//  Last but not least, write the data to the file.
	//
	DWORD bytesWritten;
	if (!WriteFile(FileHandle, waveFileData, waveFileSize, &bytesWritten, NULL))
	{
		printf("Unable to write wave file: %d\n", GetLastError());
		delete[]waveFileData;
		return false;
	}

	if (bytesWritten != waveFileSize)
	{
		printf("Failed to write entire wave file\n");
		delete[]waveFileData;
		return false;
	}
	delete[]waveFileData;
	return true;
}

//
//  Write the captured wave data to an output file so that it can be examined later.
//
void SaveWaveData(BYTE *CaptureBuffer, size_t BufferSize, const WAVEFORMATEX *WaveFormat)
{
	HRESULT hr = NOERROR;

	SYSTEMTIME st;
	GetLocalTime(&st);
	char waveFileName[_MAX_PATH] = { 0 };
	sprintf(waveFileName, ".\\WAS_%04d-%02d-%02d_%02d_%02d_%02d_%02d.wav",
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	HANDLE waveHandle = CreateFile(waveFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	if (waveHandle != INVALID_HANDLE_VALUE)
	{
		if (WriteWaveFile(waveHandle, CaptureBuffer, BufferSize, WaveFormat))
		{
			printf("Successfully wrote WAVE data to %s\n", waveFileName);
		}
		else
		{
			printf("Unable to write wave file\n");
		}
		CloseHandle(waveHandle);
	}
	else
	{
		printf("Unable to open output WAV file %s: %d\n", waveFileName, GetLastError());
	}

}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt);
static int select_sample_rate(const AVCodec *codec);

BOOL AdjustFormatTo16Bits(WAVEFORMATEX *pwfx)
{
	BOOL bRet(FALSE);

	if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
	{
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

		bRet = TRUE;
	}
	else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
		{
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			pEx->Samples.wValidBitsPerSample = 16;
			pwfx->wBitsPerSample = 16;
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

			bRet = TRUE;
		}
	}

	return bRet;
}

#define DEF_CAPTURE_MIC

HRESULT capture_audio()
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	UINT32 packetLength = 0;
	BOOL bDone = FALSE;
	BYTE *pData;
	DWORD flags;

		/*CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)*/

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr)

		hr = pEnumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, &pDevice);
	EXIT_ON_ERROR(hr)

		hr = pDevice->Activate(
			IID_IAudioClient, CLSCTX_ALL,
			NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr)

		//AdjustFormatTo16Bits(pwfx);

		hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_LOOPBACK| AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			hnsRequestedDuration,
			0,
			pwfx,
			NULL);
	EXIT_ON_ERROR(hr)

		size_t persample_size = (pwfx->wBitsPerSample / 8) * pwfx->nChannels;

		// Get the size of the allocated buffer.
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->GetService(
			IID_IAudioCaptureClient,
			(void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)

		// Calculate the actual duration of the allocated buffer.
		hnsActualDuration = (double)REFTIMES_PER_SEC *
		bufferFrameCount / pwfx->nSamplesPerSec;


	av_register_all();

	AVIOContext *output_io_context = NULL;
	AVFormatContext *output_format_context = NULL;
	AVCodecContext *output_codec_context = NULL;
	AVStream *output_stream = NULL;
	AVCodec *output_codec = NULL;

	const char* out_file = "tdjm.aac";

	if (avio_open(&output_io_context, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file!\n");
		return -1;
	}

	output_format_context = avformat_alloc_context();
	if (output_format_context == NULL) {
		return -1;
	}

	output_format_context->pb = output_io_context;

	output_format_context->oformat = av_guess_format(NULL, out_file, NULL);

	output_format_context->url = av_strdup(out_file);

	if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_AAC)))
		return -1;

	int is_support = check_sample_fmt(output_codec, AV_SAMPLE_FMT_S16);
	int selected_sample = select_sample_rate(output_codec);

	output_stream = avformat_new_stream(output_format_context, NULL);
	if (!output_stream)
		return -1;

	output_codec_context = avcodec_alloc_context3(output_codec);
	if (output_codec_context == NULL)
		return -1;

	output_codec_context->channels = 2;//av_get_channel_layout_nb_channels(pEncoderCtx->channel_layout);
	output_codec_context->channel_layout = av_get_default_channel_layout(2); //AV_CH_LAYOUT_STEREO;
	output_codec_context->sample_rate = pwfx->nSamplesPerSec;//48000
	output_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
	output_codec_context->bit_rate = 96000;

	output_codec_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	output_codec_context->time_base.den = pwfx->nSamplesPerSec;
	output_codec_context->time_base.num = 1;


	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
		output_format_context->oformat->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}	

	if (avcodec_open2(output_codec_context, output_codec, NULL) < 0)
		throw std::exception("open audio encoder failed");

	if (avcodec_parameters_from_context(output_stream->codecpar, output_codec_context) < 0)
		return -1;

	AVFrame *resampled_frame = av_frame_alloc();
	resampled_frame->nb_samples = output_codec_context->frame_size;
	resampled_frame->channel_layout = output_codec_context->channel_layout;
	resampled_frame->format = output_codec_context->sample_fmt;
	resampled_frame->sample_rate = output_codec_context->sample_rate;

	int resampled_size = av_samples_get_buffer_size(NULL, output_codec_context->channels, output_codec_context->frame_size, output_codec_context->sample_fmt, 0);

	uint8_t *resampled_buf = (uint8_t*)av_malloc(resampled_size);
	if (avcodec_fill_audio_frame(resampled_frame, output_codec_context->channels, output_codec_context->sample_fmt, resampled_buf, resampled_size, 0) < 0)
		return -1;

	AVFrame *sample_frame = av_frame_alloc();
	sample_frame->nb_samples = 1024;
	sample_frame->channel_layout = av_get_default_channel_layout(pwfx->nChannels);
	sample_frame->format = AV_SAMPLE_FMT_FLT;
	sample_frame->sample_rate = pwfx->nSamplesPerSec;

	int sample_size = av_samples_get_buffer_size(NULL, pwfx->nChannels, 1024, AV_SAMPLE_FMT_FLT, 0);
	uint8_t *sample_buf = (uint8_t*)av_malloc(sample_size);

	avcodec_fill_audio_frame(sample_frame, pwfx->nChannels, AV_SAMPLE_FMT_FLT, sample_buf, sample_size, 0);

	AVPacket pkt;

	HANDLE hAudioSamplesReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hAudioSamplesReadyEvent == NULL)
	{
		printf("Unable to create samples ready event: %d.\n", GetLastError());
		goto Exit;
	}


	hr = pAudioClient->SetEventHandle(hAudioSamplesReadyEvent);
	if (FAILED(hr))
	{
		printf("Unable to set ready event: %x.\n", hr);
		return false;
	}

	//init resampler
	SwrContext *resample_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP, 48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 48000, 0, NULL);
	int error = swr_init(resample_ctx);
	if (error < 0)
		return false;

	//init FIFO buffer

	//start recording

	avformat_write_header(output_format_context, NULL);

	hr = pAudioClient->Start();
	EXIT_ON_ERROR(hr)

		int pcmInBuffer = 0;
	int copiedPcm = 0;
	int gotPacket = 0;
	int index = 0;

	HANDLE waitArray[3];
	waitArray[0] = hAudioSamplesReadyEvent;

	uint64_t nextptx = 0;

	while (bDone == false)
	{
		DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, INFINITE);
		switch (waitResult)
		{
		case WAIT_OBJECT_0 + 0:     // _AudioSamplesReadyEvent
			hr = pCaptureClient->GetNextPacketSize(&packetLength);

			while (packetLength != 0)
			{
				// Get the available data in the shared buffer.
				hr = pCaptureClient->GetBuffer(
					&pData,
					&numFramesAvailable,
					&flags, NULL, NULL);

				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
				{
					pData = NULL;  // Tell CopyData to write silence.
				}

				// Copy the available capture data to the audio sink.

				if (pData != NULL) {

					copiedPcm = min(sample_size - pcmInBuffer, numFramesAvailable*persample_size);
					if (copiedPcm > 0) {
						memcpy(sample_buf + pcmInBuffer, pData, copiedPcm);
						pcmInBuffer += copiedPcm;
					}

					if (pcmInBuffer == sample_size) {//got one frame ,encode

						int ret = swr_convert(resample_ctx, resampled_frame->data, output_codec_context->frame_size, (const uint8_t**)sample_frame->data, 1024);
						if (ret <= 0)
							return -1;

						av_init_packet(&pkt);
						resampled_frame->pts = nextptx;
						nextptx += resampled_frame->nb_samples;

						int error = avcodec_send_frame(output_codec_context, resampled_frame);

						if (error == 0) {
							error = avcodec_receive_packet(output_codec_context, &pkt);
							if (error == 0) {
								
								av_write_frame(output_format_context, &pkt);

								index++;

								printf("index:%d\r\n", index);
							}
							av_packet_unref(&pkt);
						}
						pcmInBuffer = 0;
					}

					if (numFramesAvailable*persample_size - copiedPcm > 0)
					{
						memcpy(sample_buf + pcmInBuffer, pData + copiedPcm, numFramesAvailable*persample_size - copiedPcm);

						pcmInBuffer += numFramesAvailable*persample_size - copiedPcm;
					}

					if (index > 2000) {
						printf("pcm still in buffer:%d\r\n", pcmInBuffer);
						bDone = true;
						break;
					}
				}

				hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
				EXIT_ON_ERROR(hr)

					hr = pCaptureClient->GetNextPacketSize(&packetLength);
				EXIT_ON_ERROR(hr)
			}

			break;
		} // end of 'switch (waitResult)'

	} // end of 'while (stillPlaying)'


		av_write_trailer(output_format_context);

		if (output_codec_context)
			avcodec_free_context(&output_codec_context);
		if (output_format_context) {
			avio_closep(&output_format_context->pb);
			avformat_free_context(output_format_context);
		}

	hr = pAudioClient->Stop();  // Stop recording.
	EXIT_ON_ERROR(hr)

		Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)

		return hr;

}

HRESULT cpature_wave()
{
	HRESULT hr;

	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice           *pDevice = NULL;
	IAudioClient        *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX        *pwfx = NULL;

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	UINT32         bufferFrameCount;
	UINT32         numFramesAvailable;

	BYTE           *pData;
	UINT32         packetLength = 0;
	DWORD          flags;

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("Unable to initialize COM in thread: %x\n", hr);
		return hr;
	}

	hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
		NULL,
		CLSCTX_ALL,
		IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr)

#ifdef DEF_CAPTURE_MIC
		hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &pDevice);
	//hr = pEnumerator->GetDefaultAudioEndpoint(eCapture,  eMultimedia, &pDevice);
#else 
		hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
#endif	

	EXIT_ON_ERROR(hr)

		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr)

		AdjustFormatTo16Bits(pwfx);



#ifdef DEF_CAPTURE_MIC
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
#else
	/*
	The AUDCLNT_STREAMFLAGS_LOOPBACK flag enables loopback recording.
	In loopback recording, the audio engine copies the audio stream
	that is being played by a rendering endpoint device into an audio endpoint buffer
	so that a WASAPI client can capture the stream.
	If this flag is set, the IAudioClient::Initialize method attempts to open a capture buffer on the rendering device.
	This flag is valid only for a rendering device
	and only if the Initialize call sets the ShareMode parameter to AUDCLNT_SHAREMODE_SHARED.
	Otherwise the Initialize call will fail.
	If the call succeeds,
	the client can call the IAudioClient::GetService method
	to obtain an IAudioCaptureClient interface on the rendering device.
	For more information, see Loopback Recording.
	*/
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK|
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
#endif
	EXIT_ON_ERROR(hr)

		int nFrameSize = (pwfx->wBitsPerSample / 8) * pwfx->nChannels;

		REFERENCE_TIME hnsStreamLatency;
	hr = pAudioClient->GetStreamLatency(&hnsStreamLatency);
	EXIT_ON_ERROR(hr)


		REFERENCE_TIME hnsDefaultDevicePeriod;
	REFERENCE_TIME hnsMinimumDevicePeriod;
	hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, &hnsMinimumDevicePeriod);
	EXIT_ON_ERROR(hr)


		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)
		std::cout << std::endl << "GetBufferSize        : " << bufferFrameCount << std::endl;

	// SetEventHandle
	//////////////////////////////////////////////////////////////////////////
	HANDLE hAudioSamplesReadyEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (hAudioSamplesReadyEvent == NULL)
	{
		printf("Unable to create samples ready event: %d.\n", GetLastError());
		goto Exit;
	}


	hr = pAudioClient->SetEventHandle(hAudioSamplesReadyEvent);
	if (FAILED(hr))
	{
		printf("Unable to set ready event: %x.\n", hr);
		return false;
	}
	//////////////////////////////////////////////////////////////////////////

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);

	EXIT_ON_ERROR(hr)

		hr = pAudioClient->Start();  // Start recording.
	EXIT_ON_ERROR(hr)

		printf("\nAudio Capture begin...\n\n");

	int  nCnt = 0;

	size_t nCaptureBufferSize = 8 * 1024 * 1024;
	size_t nCurrentCaptureIndex = 0;

	BYTE *pbyCaptureBuffer = new (std::nothrow) BYTE[nCaptureBufferSize];

	HANDLE waitArray[3];
	waitArray[0] = hAudioSamplesReadyEvent;

	bool stillPlaying = true;

	// Each loop fills about half of the shared buffer.
	while (stillPlaying)
	{
		DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, INFINITE);
		switch (waitResult)
		{
		case WAIT_OBJECT_0 + 0:     // _AudioSamplesReadyEvent
			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			EXIT_ON_ERROR(hr)

				printf("%06d # _AudioSamplesReadyEvent packetLength:%06u \n", nCnt, packetLength);

			while (packetLength != 0)
			{
				// Get the available data in the shared buffer.
				hr = pCaptureClient->GetBuffer(&pData,
					&numFramesAvailable,
					&flags, NULL, NULL);
				EXIT_ON_ERROR(hr)


					nCnt++;

				// test flags
				//////////////////////////////////////////////////////////////////////////
				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
				{
					printf("AUDCLNT_BUFFERFLAGS_SILENT \n");
				}

				if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
				{
					printf("%06d # AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY \n", nCnt);
				}
				//////////////////////////////////////////////////////////////////////////

				UINT32 framesToCopy = min(numFramesAvailable, static_cast<UINT32>((nCaptureBufferSize - nCurrentCaptureIndex) / nFrameSize));
				if (framesToCopy != 0)
				{
					//
					//  The flags on capture tell us information about the data.
					//
					//  We only really care about the silent flag since we want to put frames of silence into the buffer
					//  when we receive silence.  We rely on the fact that a logical bit 0 is silence for both float and int formats.
					//
					if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
					{
						//
						//  Fill 0s from the capture buffer to the output buffer.
						//
						ZeroMemory(&pbyCaptureBuffer[nCurrentCaptureIndex], framesToCopy*nFrameSize);
					}
					else
					{
						//
						//  Copy data from the audio engine buffer to the output buffer.
						//
						CopyMemory(&pbyCaptureBuffer[nCurrentCaptureIndex], pData, framesToCopy*nFrameSize);

						printf("Get: %d\r\n", framesToCopy*nFrameSize);
					}
					//
					//  Bump the capture buffer pointer.
					//
					nCurrentCaptureIndex += framesToCopy*nFrameSize;
				}

				hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
				EXIT_ON_ERROR(hr)

					hr = pCaptureClient->GetNextPacketSize(&packetLength);
				EXIT_ON_ERROR(hr)

					UINT32 ui32NumPaddingFrames;
				hr = pAudioClient->GetCurrentPadding(&ui32NumPaddingFrames);
				EXIT_ON_ERROR(hr)
					if (0 != ui32NumPaddingFrames)
					{
						printf("GetCurrentPadding : %6u\n", ui32NumPaddingFrames);
					}
				//////////////////////////////////////////////////////////////////////////

				if (nCnt == 1000)
				{
					stillPlaying = false;
					break;
				}

			} // end of 'while (packetLength != 0)'

			break;
		} // end of 'switch (waitResult)'

	} // end of 'while (stillPlaying)'

	  //
	  //  We've now captured our wave data.  Now write it out in a wave file.
	  //
	SaveWaveData(pbyCaptureBuffer, nCurrentCaptureIndex, pwfx);

	printf("\nAudio Capture Done.\n");

	hr = pAudioClient->Stop();  // Stop recording.
	EXIT_ON_ERROR(hr)

		Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)

		CoUninitialize();

	if (pbyCaptureBuffer)
	{
		delete[] pbyCaptureBuffer;
		pbyCaptureBuffer = NULL;
	}

	if (hAudioSamplesReadyEvent)
	{
		CloseHandle(hAudioSamplesReadyEvent);
		hAudioSamplesReadyEvent = NULL;
	}
}

/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat *p = codec->sample_fmts;

	while (*p != AV_SAMPLE_FMT_NONE) {
		if (*p == sample_fmt)
			return 1;
		p++;
	}
	return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
	const int *p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return 44100;

	p = codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
			best_samplerate = *p;
		p++;
	}
	return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec)
{
	const uint64_t *p;
	uint64_t best_ch_layout = 0;
	int best_nb_channels = 0;

	if (!codec->channel_layouts)
		return AV_CH_LAYOUT_STEREO;

	p = codec->channel_layouts;
	while (*p) {
		int nb_channels = av_get_channel_layout_nb_channels(*p);

		if (nb_channels > best_nb_channels) {
			best_ch_layout = *p;
			best_nb_channels = nb_channels;
		}
		p++;
	}
	return best_ch_layout;
}

int main1()
{
	//capture_audio();
	//test_transcode();
	cpature_wave();

	system("pause");

    return 0;
}

