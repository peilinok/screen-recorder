#include "record_audio_wasapi.h"

#include "error_define.h"
#include "log_helper.h"

#ifdef _WIN32


#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#endif // _WIN32


namespace am {

	record_audio_wasapi::record_audio_wasapi()
	{
		_co_inited = false;

		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (hr != S_OK)
			al_debug("%s,error:%ld",err2str(AE_CO_INITED_FAILED), GetLastError());

		_co_inited = (hr == S_OK || hr == S_FALSE);//if already initialize will return S_FALSE

		_wfex = NULL;
		_enumerator = nullptr;
		_device = nullptr;
		_client = nullptr;
		_capture = nullptr;

		_ready_event = NULL;
	}

	record_audio_wasapi::~record_audio_wasapi()
	{
		stop();

		clean_wasapi();

		if(_co_inited == true)
			CoUninitialize();
	}

	int record_audio_wasapi::init(const std::string & device_name)
	{
		int error = AE_NO;

		if (_co_inited == false) {
			return AE_CO_INITED_FAILED;
		}

		if (_inited == true) {
			return AE_NO;
		}

		do {
			HRESULT hr = CoCreateInstance(
				__uuidof(MMDeviceEnumerator),
				NULL,
				CLSCTX_ALL,
				__uuidof(IMMDeviceEnumerator),
				(void **)&_enumerator);

			if (hr != S_OK) {
				error = AE_CO_CREATE_FAILED;
				break;
			}

			hr = _enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_device);
			if (hr != S_OK) {
				error = AE_CO_GETENDPOINT_FAILED;
				break;
			}

			hr = _device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_client);
			if (hr != S_OK) {
				error = AE_CO_ACTIVE_DEVICE_FAILED;
				break;
			}

			hr = _client->GetMixFormat(&_wfex);
			if (hr != S_OK) {
				error = AE_CO_GET_FORMAT_FAILED;
				break;
			}

			hr = _client->Initialize(
				AUDCLNT_SHAREMODE_SHARED,
				AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				REFTIMES_PER_SEC,
				0,
				_wfex,
				NULL);

			if (hr != S_OK) {
				error = AE_CO_AUDIOCLIENT_INIT_FAILED;
				break;
			}

			hr = _client->GetService(__uuidof(IAudioCaptureClient), (void**)&_capture);
			if (hr != S_OK) {
				error = AE_CO_GET_CAPTURE_FAILED;
				break;
			}

			_ready_event = CreateEvent(NULL,FALSE,FALSE,NULL);
			if (!_ready_event) {
				error = AE_CO_CREATE_EVENT_FAILED;
				break;
			}

			hr = _client->SetEventHandle(_ready_event);
			if (hr != S_OK) {
				error = AE_CO_SET_EVENT_FAILED;
				break;
			}

			_sample_rate = _wfex->nSamplesPerSec;
			_bit_rate = _wfex->nAvgBytesPerSec;
			_bit_per_sample = _wfex->wBitsPerSample;
			_channel_num = _wfex->nChannels;
			_fmt = AV_SAMPLE_FMT_FLT;

			_inited = true;

		} while (0);

		if (error != AE_NO) {
			al_debug("%s,error:%ld", err2str(error), GetLastError());
			clean_wasapi();
		}

		return error;
	}

	int record_audio_wasapi::start()
	{
		if (_running == true) {
			al_warn("audio record is already running");
			return AE_NO;
		}

		if (_inited == false)
			return AE_NEED_INIT;

		HRESULT hr = _client->Start();
		if (hr != S_OK) {
			al_debug("%s,error:%ld", err2str(AE_CO_START_FAILED), GetLastError());
			return AE_CO_START_FAILED;
		}

		_running = true;
		_thread = std::thread(std::bind(&record_audio_wasapi::record_func, this));

		return AE_NO;
	}

	int record_audio_wasapi::pause()
	{
		return AE_NO;
	}

	int record_audio_wasapi::resume()
	{
		return AE_NO;
	}

	int record_audio_wasapi::stop()
	{
		if (_client)
			_client->Stop();

		_running = false;
		if (_thread.joinable())
			_thread.join();

		return AE_NO;
	}

	const AVRational & record_audio_wasapi::get_time_base()
	{
		return{ 1,90000 };
	}

	int64_t record_audio_wasapi::get_start_time()
	{
		return 0;
	}


	void record_audio_wasapi::record_func()
	{
		HANDLE events[3] = { NULL };
		events[0] = _ready_event;

		DWORD ret = WAIT_TIMEOUT;
		HRESULT hr = S_OK;

		unsigned int packet_num = 0;
		unsigned int frame_num = 0;
		DWORD flags = 0;
		BYTE *data = NULL;

		int frame_size = _bit_per_sample / 8 * _channel_num;

		while (_running)
		{
			ret = WaitForMultipleObjects(1, events, FALSE, 500);
			if (ret != WAIT_OBJECT_0)
				continue;

			hr = _capture->GetNextPacketSize(&packet_num);
			while (packet_num != 0 && _running == true)
			{
				hr = _capture->GetBuffer(&data, &frame_num, &flags, NULL, NULL);
				if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {//tell to copy silence data
					//data = NULL;
				}

				if (_on_data && data) {
					//_on_data(data, frame_num*frame_size, _cb_extra_index);
				}

				if(data)
					hr = _capture->ReleaseBuffer(frame_num);
				hr = _capture->GetNextPacketSize(&packet_num);
			}

		}//while(_running)
	}

	bool record_audio_wasapi::adjust_format_2_16bits(WAVEFORMATEX *pwfx)
	{
		bool ret = false;

		if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		{
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->wBitsPerSample = 16;
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

			ret = TRUE;
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

				ret = TRUE;
			}
		}

		return ret;
	}

	void record_audio_wasapi::clean_wasapi()
	{
		if (_wfex)
			CoTaskMemFree(_wfex);
		_wfex = NULL;

		if (_enumerator)
			_enumerator->Release();
		_enumerator = nullptr;

		if (_device)
			_device->Release();
		_device = nullptr;

		if (_client)
			_client->Release();
		_client = nullptr;

		if (_capture)
			_capture->Release();
		_capture = nullptr;

		if (_ready_event)
			CloseHandle(_ready_event);
		_ready_event = NULL;

		if(_co_inited == true)
			CoUninitialize();

		_co_inited = false;
		_inited = false;
	}


}
