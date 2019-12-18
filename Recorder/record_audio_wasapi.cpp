#include "record_audio_wasapi.h"

#include <string>

#include "error_define.h"
#include "log_helper.h"
#include "utils_string.h"

#ifdef _WIN32

#define NS_PER_SEC 1000000000
#define REFTIMES_PER_SEC  NS_PER_SEC/100            //100ns per buffer unit

#endif // _WIN32


namespace am {

	record_audio_wasapi::record_audio_wasapi()
	{
		_co_inited = false;

		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (hr != S_OK)
			al_debug("%s,error:%ld",err2str(AE_CO_INITED_FAILED), GetLastError());

		_co_inited = (hr == S_OK || hr == S_FALSE);//if already initialize will return S_FALSE

		_is_default = false;

		_wfex = NULL;
		_enumerator = nullptr;
		_device = nullptr;
		_capture_client = nullptr;
		_capture = nullptr;
		_render = nullptr;
		_render_client = nullptr;

		_capture_sample_count = 0;
		_render_sample_count = 0;

		_ready_event = NULL;
		_stop_event = NULL;
		_render_event = NULL;

		_start_time = 0;
		_pre_pts = 0;
	}

	record_audio_wasapi::~record_audio_wasapi()
	{
		stop();

		clean_wasapi();

		if(_co_inited == true)
			CoUninitialize();
	}

	void get_device_info(IMMDevice *device) {
		HRESULT resSample;
		IPropertyStore *store = nullptr;
		PWAVEFORMATEX deviceFormatProperties;
		PROPVARIANT prop;
		resSample = device->OpenPropertyStore(STGM_READ, &store);
		if (!FAILED(resSample)) {
			resSample =
				store->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
			if (!FAILED(resSample)) {
				deviceFormatProperties =
					(PWAVEFORMATEX)prop.blob.pBlobData;
				std::string device_sample = std::to_string(
					deviceFormatProperties->nSamplesPerSec);
			}
		}
	}

	int record_audio_wasapi::init_render()
	{
		int error = AE_NO;

		do {
			HRESULT res = _device->Activate(__uuidof(IAudioClient),
				CLSCTX_ALL, 
				nullptr,
				(void **)&_render_client
			);

			if (FAILED(res)) {
				error = AE_CO_ACTIVE_DEVICE_FAILED;
				break;
			}

			WAVEFORMATEX *wfex;
			res = _render_client->GetMixFormat(&wfex);
			if (FAILED(res)) {
				error = AE_CO_GET_FORMAT_FAILED;
				break;
			}

			res = _render_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
				AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				REFTIMES_PER_SEC,
				0, wfex, nullptr);

			CoTaskMemFree(wfex);
			if (FAILED(res)) {
				error = AE_CO_AUDIOCLIENT_INIT_FAILED;
				break;
			}

			/* Silent loopback fix. Prevents audio stream from stopping and */
			/* messing up timestamps and other weird glitches during silence */
			/* by playing a silent sample all over again. */

			res = _render_client->GetService(__uuidof(IAudioRenderClient),
				(void **)&_render);
			if (FAILED(res)) {
				error = AE_CO_GET_CAPTURE_FAILED;
				break;
			}

			_render_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!_render_event) {
				error = AE_CO_CREATE_EVENT_FAILED;
				break;
			}
			
			res = _render_client->SetEventHandle(_render_event);
			if (FAILED(res)) {
				error = AE_CO_SET_EVENT_FAILED;
				break;
			}

			//pre fill a single silent buffer
			res = _render_client->GetBufferSize(&_render_sample_count);
			if (FAILED(res)) {
				error = AE_CO_GET_VALUE_FAILED;
				break;
			}

			uint8_t *buffer = NULL;
			res = _render->GetBuffer(_render_sample_count, &buffer);
			if (FAILED(res)) {
				error = AE_CO_GET_VALUE_FAILED;
				break;
			}

			res = _render->ReleaseBuffer(_render_sample_count, AUDCLNT_BUFFERFLAGS_SILENT);
			if (FAILED(res)) {
				error = AE_CO_RELEASE_BUFFER_FAILED;
				break;
			}
		} while (0);

		return error;
	}

	int record_audio_wasapi::init(const std::string & device_name, const std::string &device_id, bool is_input)
	{
		int error = AE_NO;

		if (_co_inited == false) {
			return AE_CO_INITED_FAILED;
		}

		if (_inited == true) {
			return AE_NO;
		}

		_device_name = device_name;
		_device_id = device_id;
		_is_input = is_input;
		_is_default = (utils_string::ascii_utf8(DEFAULT_AUDIO_INOUTPUT_ID).compare(_device_id) == 0);

		do {
			HRESULT hr = CoCreateInstance(
				__uuidof(MMDeviceEnumerator),
				NULL,
				CLSCTX_ALL,
				__uuidof(IMMDeviceEnumerator),
				(void **)&_enumerator);

			if (FAILED(hr)) {
				error = AE_CO_CREATE_FAILED;
				break;
			}

			if (_is_default) {
				hr = _enumerator->GetDefaultAudioEndpoint(
					is_input ? eCapture : eRender,
					is_input ? eCommunications : eConsole, &_device);
			}
			else {
				hr = _enumerator->GetDevice(utils_string::utf8_unicode(_device_id).c_str(), &_device);
			}

			if (hr != S_OK) {
				error = AE_CO_GETENDPOINT_FAILED;
				break;
			}

			get_device_info(_device);

			hr = _device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_capture_client);
			if (hr != S_OK) {
				error = AE_CO_ACTIVE_DEVICE_FAILED;
				break;
			}

			hr = _capture_client->GetMixFormat(&_wfex);
			if (hr != S_OK) {
				error = AE_CO_GET_FORMAT_FAILED;
				break;
			}

			DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
			if (_is_input == false)
				flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;//for output mode,event will not auto set when there is no audio rendering

			hr = _capture_client->Initialize(
				AUDCLNT_SHAREMODE_SHARED,
				flags,
				REFTIMES_PER_SEC,
				0,
				_wfex,
				NULL);

			if (hr != S_OK) {
				error = AE_CO_AUDIOCLIENT_INIT_FAILED;
				break;
			}

			if (_is_input == false) {
				error = init_render();

				if (error != AE_NO)
					break;
			}

			hr = _capture_client->GetBufferSize(&_capture_sample_count);
			if (hr != S_OK) {
				error = AE_CO_GET_VALUE_FAILED;
				break;
			}


			hr = _capture_client->GetService(__uuidof(IAudioCaptureClient), (void**)&_capture);
			if (hr != S_OK) {
				error = AE_CO_GET_CAPTURE_FAILED;
				break;
			}

			_ready_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!_ready_event) {
				error = AE_CO_CREATE_EVENT_FAILED;
				break;
			}

			_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);

			if (_is_input || !_is_input) {
				hr = _capture_client->SetEventHandle(_ready_event);
				if (hr != S_OK) {
					error = AE_CO_SET_EVENT_FAILED;
					break;
				}
			}

			_sample_rate = _wfex->nSamplesPerSec;
			_bit_rate = _wfex->nAvgBytesPerSec;
			_bit_per_sample = _wfex->wBitsPerSample;
			_channel_num = _wfex->nChannels;
			_fmt = AV_SAMPLE_FMT_FLT;//wasapi is always flt

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

		HRESULT hr = S_OK;
		
		if (!_is_input) {
			hr = _render_client->Start();
			if (FAILED(hr)) {
				al_error("%s,error:%ld", err2str(AE_CO_START_FAILED), GetLastError());
				return AE_CO_START_FAILED;
			}
		}
		
		hr = _capture_client->Start();
		if (hr != S_OK) {
			al_error("%s,error:%ld", err2str(AE_CO_START_FAILED), GetLastError());
			return AE_CO_START_FAILED;
		}

		_start_time = av_gettime_relative();

		_running = true;
		if(_is_input)
			_thread = std::thread(std::bind(&record_audio_wasapi::record_func_input, this));
		else {
			_render_thread = std::thread(std::bind(&record_audio_wasapi::render_func, this));
			_thread = std::thread(std::bind(&record_audio_wasapi::record_func_output, this));
		}
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
		_running = false;
		SetEvent(_stop_event);

		if (_render_thread.joinable())
			_render_thread.join();

		if (_thread.joinable())
			_thread.join();

		if (_capture_client)
			_capture_client->Stop();

		if (_render_client)
			_render_client->Stop();

		return AE_NO;
	}

	const AVRational & record_audio_wasapi::get_time_base()
	{
		return{ 1,AV_TIME_BASE };
	}

	int64_t record_audio_wasapi::get_start_time()
	{
		return _start_time;
	}

	void record_audio_wasapi::process_data(AVFrame *frame, uint8_t* data, uint32_t sample_count)
	{
		int sample_size = _bit_per_sample / 8 * _channel_num;

		//use relative time instead of device time
		frame->pts = av_gettime_relative();
		frame->pts -= (int64_t)sample_count * AV_TIME_BASE / (int64_t)_sample_rate;
		frame->pkt_dts = frame->pts;
		frame->pkt_pts = frame->pts;
		frame->data[0] = data;
		frame->linesize[0] = sample_count*sample_size / _channel_num;
		frame->nb_samples = sample_count;
		frame->format = _fmt;
		frame->sample_rate = _sample_rate;
		frame->channels = _channel_num;
		frame->pkt_size = sample_count*sample_size;

#if 1
		if (!_is_input) {
			static int64_t pre_pts = 0;
			al_debug("AF:%lld", frame->pts - pre_pts);
			pre_pts = frame->pts;
		}
#endif

		if (_on_data) _on_data(frame, _cb_extra_index);
	}

	bool record_audio_wasapi::do_record_input(AVFrame *frame)
	{
		HRESULT res;
		LPBYTE buffer = NULL;
		DWORD flags;

		uint64_t pos, ts;
		uint32_t sample_count = 0;

		while (_running) {
			res = _capture->GetNextPacketSize(&sample_count);

			if (FAILED(res)) {
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					al_warn("GetNextPacketSize failed: %lX", res);

				return false;
			}

			if (!sample_count)
				break;

			buffer = NULL;
			res = _capture->GetBuffer(&buffer, &sample_count, &flags, &pos, &ts);
			if (FAILED(res)) {
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					al_warn("GetBuffer failed: %lX",res);

				return false;
			}

			//input mode do not have silent data flag do nothing here
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				al_warn("input slient data %d", sample_count);
			}

			if (buffer) {
				process_data(frame, buffer, sample_count);
			}
			else {
				al_debug("input buffer invalid is");
			}

			_capture->ReleaseBuffer(sample_count);
		}

		return true;
	}

	bool record_audio_wasapi::do_record_output(AVFrame *frame)
	{
		HRESULT res;
		LPBYTE buffer = NULL;
		DWORD flags;

		uint64_t pos, ts;
		uint32_t sample_count = 0;

		while (_running) {
			res = _capture->GetNextPacketSize(&sample_count);
			if (FAILED(res)) {
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					al_warn("GetNextPacketSize failed: %lX", res);

				return false;
			}

			//when there is no pcm rendering,the capture will not return packet_size,so we put silent data(10ms)
			if (!sample_count) {
				//process_data(frame, _silent_data, _silent_sample_count);
				return true;
			}

			buffer = NULL;
			res = _capture->GetBuffer(&buffer, &sample_count, &flags, &pos, &ts);
			if (FAILED(res)) {
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					al_warn("GetBuffer failed: %lX", res);

				return false;
			}

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				//al_debug("output slient data,%d %d", sample_count, _silent_sample_count);
				process_data(frame, _silent_data, min(sample_count, _silent_sample_count));
			} 
			else {

				if (buffer) {
					process_data(frame, buffer, sample_count);
				}
				else {
					al_warn("output buffer invalid");
				}
			}

			_capture->ReleaseBuffer(sample_count);
		}

		return true;
	}

	void record_audio_wasapi::render_func()
	{
		HANDLE events[2] = { _stop_event,_render_event };

		uint8_t *pData = NULL;
		while (_running && 
			WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0
			) {
			// got "feed me" event - see how much padding there is
			//
			// padding is how much of the buffer is currently in use
			//
			// note in particular that event-driven (pull-mode) render should not
			// call GetCurrentPadding multiple times
			// in a single processing pass
			// this is in stark contrast to timer-driven (push-mode) render
			UINT32 nFramesOfPadding;
			HRESULT hr = _render_client->GetCurrentPadding(&nFramesOfPadding);
			if (FAILED(hr)) {
				break;
			}

			if (nFramesOfPadding == _render_sample_count) {
				hr = E_UNEXPECTED;
				printf("Got \"feed me\" event but IAudioClient::GetCurrentPadding reports buffer is full - glitch?\n");
				break;
			}

			hr = _render->GetBuffer(_render_sample_count - nFramesOfPadding, &pData);
			if (FAILED(hr)) {
				al_error("%s", err2str(AE_CO_GET_BUFFER_FAILED));
				break;
			}

			// *** AT THIS POINT ***
			// If you wanted to render something besides silence,
			// you would fill the buffer pData
			// with (nFramesInBuffer - nFramesOfPadding) worth of audio data
			// this should be in the same wave format
			// that the stream was initialized with
			//
			// In particular, if you didn't want to use the mix format,
			// you would need to either ask for a different format in IAudioClient::Initialize
			// or do a format conversion
			//
			// If you do, then change the AUDCLNT_BUFFERFLAGS_SILENT flags value below to 0

			// release the buffer with the silence flag
			hr = _render->ReleaseBuffer(_render_sample_count - nFramesOfPadding, AUDCLNT_BUFFERFLAGS_SILENT);
			if (FAILED(hr)) {
				al_error("IAudioRenderClient::ReleaseBuffer failed on pass %u: hr = 0x%08x - glitch?\n", 0, hr);
				break;
			}
		}
	}

	void record_audio_wasapi::record_func_input()
	{
		HANDLE events[2] = { _ready_event,_stop_event };

		DWORD ret = WAIT_TIMEOUT;

		AVFrame *frame = av_frame_alloc();

		while (_running)
		{
			ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if (ret != WAIT_OBJECT_0 && ret != WAIT_TIMEOUT)
				continue;

			if (!do_record_input(frame))
				break;

		}//while(_running)

		av_frame_free(&frame);
	}

	void record_audio_wasapi::record_func_output()
	{
		AVFrame *frame = av_frame_alloc();

		int64_t dur = 10;

		_silent_sample_count = _sample_rate * dur / 1000;
		_silent_data_size = _silent_sample_count * _bit_per_sample / 8 * _channel_num;
		_silent_data = new uint8_t[_silent_data_size];

		memset(_silent_data, 0, _silent_data_size);

		HANDLE events[2] = { _stop_event,_ready_event };

		DWORD ret = WAIT_TIMEOUT;

		while (_running)
		{
			if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0)
				break;

			if (!do_record_output(frame))
				break;
		}//while(_running)

		av_frame_free(&frame);

		delete[] _silent_data;
 	}

	//no need any more,coz wasapi is always flt
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

		if (_capture_client)
			_capture_client->Release();
		_capture_client = nullptr;

		if (_render_client)
			_render_client->Release();
		_render_client = nullptr;

		if (_capture)
			_capture->Release();
		_capture = nullptr;

		if (_render)
			_render->Release();
		_render = nullptr;

		if (_ready_event)
			CloseHandle(_ready_event);
		_ready_event = NULL;

		if (_stop_event)
			CloseHandle(_stop_event);
		_stop_event = NULL;

		if (_render_event)
			CloseHandle(_render_event);
		_render_event = NULL;

		if(_co_inited == true)
			CoUninitialize();

		_co_inited = false;
		_inited = false;
	}


}
