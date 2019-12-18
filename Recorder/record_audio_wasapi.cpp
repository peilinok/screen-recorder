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
				flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

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

			//For ouotput mode,ready event will not signal when there is nothing rendering
			//We run a render thread and render silent pcm data all time
			if (!_is_input) {
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
			if (!_stop_event) {
				error = AE_CO_CREATE_EVENT_FAILED;
				break;
			}

			hr = _capture_client->SetEventHandle(_ready_event);
			if (hr != S_OK) {
				error = AE_CO_SET_EVENT_FAILED;
				break;
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
		if(!_is_input) {
			_render_thread = std::thread(std::bind(&record_audio_wasapi::render_loop, this));
		}

		_thread = std::thread(std::bind(&record_audio_wasapi::record_loop, this));

		return AE_NO;
	}

	int record_audio_wasapi::pause()
	{
		_paused = true;
		return AE_NO;
	}

	int record_audio_wasapi::resume()
	{
		_paused = false;
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

		frame->pts = av_gettime_relative();
		frame->pts -= (int64_t)sample_count * AV_TIME_BASE / (int64_t)_sample_rate;
		frame->pkt_dts = frame->pts;
		frame->nb_samples = sample_count;
		frame->format = _fmt;
		frame->sample_rate = _sample_rate;
		frame->channels = _channel_num;
		frame->pkt_size = sample_count*sample_size;

		av_samples_fill_arrays(frame->data, frame->linesize, data, _channel_num, sample_count, _fmt, 1);

		if (_on_data) _on_data(frame, _cb_extra_index);
	}

	int record_audio_wasapi::do_record(AVFrame *frame)
	{
		HRESULT res = S_OK;
		LPBYTE buffer = NULL;
		DWORD flags = 0;
		uint32_t sample_count = 0;
		int error = AE_NO;

		while (_running) {
			res = _capture->GetNextPacketSize(&sample_count);

			if (FAILED(res)) {
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					al_error("GetNextPacketSize failed: %lX", res);
				error = AE_CO_GET_PACKET_FAILED;
				break;
			}

			if (!sample_count)
				break;

			buffer = NULL;
			res = _capture->GetBuffer(&buffer, &sample_count, &flags, NULL, NULL);
			if (FAILED(res)) {
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					al_error("GetBuffer failed: %lX",res);
				error = AE_CO_GET_BUFFER_FAILED;
				break;
			}

			//input mode do not have silent data flag do nothing here
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				al_warn("on slient data %d", sample_count);
			}

			if (buffer) {
				process_data(frame, buffer, sample_count);
			}
			else {
				al_error("buffer invalid is");
			}

			_capture->ReleaseBuffer(sample_count);
		}

		return error;
	}

	void record_audio_wasapi::render_loop()
	{
		HANDLE events[2] = { _stop_event,_render_event };

		HRESULT res = S_OK;
		uint8_t *pData = NULL;
		uint32_t padding_count = 0;

		while (_running && 
			WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0
			) {
			
			res = _render_client->GetCurrentPadding(&padding_count);
			if (FAILED(res)) {
				break;
			}

			if (padding_count == _render_sample_count) {
				if (_on_error) _on_error(AE_CO_PADDING_UNEXPECTED, _cb_extra_index);
				break;
			}

			res = _render->GetBuffer(_render_sample_count - padding_count, &pData);
			if (FAILED(res)) {
				if (_on_error) _on_error(AE_CO_GET_BUFFER_FAILED, _cb_extra_index);
				break;
			}

			res = _render->ReleaseBuffer(_render_sample_count - padding_count, AUDCLNT_BUFFERFLAGS_SILENT);
			if (FAILED(res)) {
				if (_on_error) _on_error(AE_CO_RELEASE_BUFFER_FAILED, _cb_extra_index);
				break;
			}
		}
	}

	void record_audio_wasapi::record_loop()
	{
		AVFrame *frame = av_frame_alloc();

		HANDLE events[2] = { _stop_event,_ready_event };

		int error = AE_NO;
		while (_running)
		{
			if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0)
				break;

			if ((error = do_record(frame)) != AE_NO) {
				if (_on_error) _on_error(error, _cb_extra_index);
				break;
			}
		}//while(_running)

		av_frame_free(&frame);
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
