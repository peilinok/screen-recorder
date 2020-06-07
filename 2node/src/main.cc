#include <node.h>
#include <node_buffer.h>
#include <uv.h>
#include <nan.h>

#ifdef WIN32
#include "../platform/win32/export.h"
#elif __MAC__
#include "../platform/mac/export.h"
#define PTHREAD_MUTEX_RECURSIVE_NP 1
#endif

#include <map>
#include <queue>
#include <string>
#include <Windows.h>

namespace recorder
{
	using namespace v8;
	using namespace node;

	#define AMLOG(X) printf("%s:%d %s\r\n",__FILE__,__LINE__,X);

	class Locker
	{
	public:
		Locker()
		{
#ifdef WIN32
			InitializeCriticalSection(&m_csLock);
#else
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
			pthread_mutex_init(&m_csLock, &attr);
#endif
		}
		virtual ~Locker()
		{
#ifdef WIN32
			DeleteCriticalSection(&m_csLock);
#else
			pthread_mutex_destroy(&m_csLock);
#endif
		}

		long Lock()
		{
#ifdef WIN32
			EnterCriticalSection(&m_csLock);
			return m_csLock.LockCount;
#else
			pthread_mutex_lock(&m_csLock);
			return 1;
#endif
		}
		long Unlock()
		{
#ifdef WIN32
			LeaveCriticalSection(&m_csLock);
			return m_csLock.LockCount;
#else
			pthread_mutex_unlock(&m_csLock);
			return 1;
#endif
		}

	private:
#ifdef WIN32
		CRITICAL_SECTION m_csLock;
#else
		pthread_mutex_t m_csLock;
#endif
	};

	std::string unicode_utf8(const std::wstring & wstr)
	{
		int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
		std::string ret_str = pAssii;
		free(pAssii);
		return ret_str;
	}

	std::wstring utf8_unicode(const std::string & utf8)
	{
		int unicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
		wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, pUnicode, unicodeLen);
		std::wstring ret_str = pUnicode;
		free(pUnicode);
		return ret_str;
	}

	std::wstring ascii_unicode(const std::string & str)
	{
		int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);

		wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);

		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);

		std::wstring ret_str = pUnicode;

		free(pUnicode);

		return ret_str;
	}

	std::string unicode_ascii(const std::wstring & wstr)
	{
		int ansiiLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
		std::string ret_str = pAssii;
		free(pAssii);
		return ret_str;
	}

	std::string ascii_utf8(const std::string & str)
	{
		return unicode_utf8(ascii_unicode(str));
	}

	std::string utf8_ascii(const std::string & utf8)
	{
		return unicode_ascii(utf8_unicode(utf8));
	}

	Local<String> utf8_v8string(Isolate *isolate,const char *utf8) 
	{
#if NODE_VERSION_AT_LEAST(12,0,0)
		return String::NewFromUtf8(isolate, utf8).ToLocalChecked();
#elif NODE_VERSION_AT_LEAST(10,0,0)
		return String::NewFromUtf8(isolate, utf8);
#else
		return String::NewFromUtf8(isolate, utf8);
#endif
	}

	bool CheckParamCount(unsigned int nLength, unsigned int nCount) {
		if (nLength != nCount) {
			Isolate* isolate = Isolate::GetCurrent();
			isolate->ThrowException(Exception::TypeError(
				utf8_v8string(isolate,"Wrong number of arguments")));
			return false;
		}
		return true;
	}

	bool CheckParamValid(Local<Value> value, const char* strType) {
		bool bOK = true;
		if (strType) {
			if (strcmp(strType, "string") == 0)
				bOK = value->IsString();
			else if (strcmp(strType, "uint32") == 0)
				bOK = value->IsUint32();
			else if (strcmp(strType, "int32") == 0)
				bOK = value->IsInt32();
			else if (strcmp(strType, "bool") == 0)
				bOK = value->IsBoolean();
			else if (strcmp(strType, "buffer") == 0)
				bOK = Buffer::HasInstance(value);
			else if (strcmp(strType, "function") == 0)
				bOK = value->IsFunction();
		}
		if (!bOK) {
			Isolate* isolate = Isolate::GetCurrent();
			isolate->ThrowException(Exception::TypeError(
				utf8_v8string(isolate, "Wrong arguments")));
		}
		return bOK;
	}

#define CHECK_PARAM_COUNT(nCount) \
	if(!CheckParamCount(args.Length(), nCount)) \
		return;

#define CHECK_PARAM_VALID(value, type) \
	if(!CheckParamValid(value, type)) \
		return;

#define CHECK_PARAM_TYPE8(type1, type2, type3, type4, type5, type6, type7,type8) \
	if (type1 != NULL) \
		CHECK_PARAM_VALID(args[0], type1); \
	if (type2 != NULL) \
		CHECK_PARAM_VALID(args[1], type2); \
	if (type3 != NULL) \
		CHECK_PARAM_VALID(args[2], type3); \
	if (type4 != NULL) \
		CHECK_PARAM_VALID(args[3], type4); \
	if (type5 != NULL) \
		CHECK_PARAM_VALID(args[4], type5); \
	if (type6 != NULL) \
		CHECK_PARAM_VALID(args[5], type6); \
  if (type7 != NULL) \
		CHECK_PARAM_VALID(args[6], type7); \
	if (type8 != NULL) \
		CHECK_PARAM_VALID(args[7], type8);

#define CHECK_PARAM_TYPE7(type1, type2, type3, type4, type5, type6, type7) \
	if (type1 != NULL) \
		CHECK_PARAM_VALID(args[0], type1); \
	if (type2 != NULL) \
		CHECK_PARAM_VALID(args[1], type2); \
	if (type3 != NULL) \
		CHECK_PARAM_VALID(args[2], type3); \
	if (type4 != NULL) \
		CHECK_PARAM_VALID(args[3], type4); \
	if (type5 != NULL) \
		CHECK_PARAM_VALID(args[4], type5); \
	if (type6 != NULL) \
		CHECK_PARAM_VALID(args[5], type6); \
  if (type7 != NULL) \
		CHECK_PARAM_VALID(args[6], type7);

#define CHECK_PARAM_TYPE6(type1, type2, type3, type4, type5, type6) \
	if (type1 != NULL) \
		CHECK_PARAM_VALID(args[0], type1); \
	if (type2 != NULL) \
		CHECK_PARAM_VALID(args[1], type2); \
	if (type3 != NULL) \
		CHECK_PARAM_VALID(args[2], type3); \
	if (type4 != NULL) \
		CHECK_PARAM_VALID(args[3], type4); \
	if (type5 != NULL) \
		CHECK_PARAM_VALID(args[4], type5); \
	if (type6 != NULL) \
		CHECK_PARAM_VALID(args[5], type6);

#define CHECK_PARAM_TYPE5(type1, type2, type3, type4, type5) \
	CHECK_PARAM_TYPE6(type1, type2, type3, type4, type5, NULL)

#define CHECK_PARAM_TYPE4(type1, type2, type3, type4) \
	CHECK_PARAM_TYPE5(type1, type2, type3, type4, NULL)

#define CHECK_PARAM_TYPE3(type1, type2, type3) \
	CHECK_PARAM_TYPE4(type1, type2, type3, NULL)

#define CHECK_PARAM_TYPE2(type1, type2) \
	CHECK_PARAM_TYPE3(type1, type2, NULL)

#define CHECK_PARAM_TYPE1(type1) \
	CHECK_PARAM_TYPE2(type1, NULL)

	typedef enum {
		uvcb_type_duration = 1,
		uvcb_type_error,
		uvcb_type_device_change,
		uvcb_type_preview_audio,
		uvcb_type_preview_yuv,
		uvcb_type_remux_progress,
		uvcb_type_remux_state,
	}uvCallbackType;

	typedef struct {
		uint64_t duration;
	}uvCallBackDataDruation;

	typedef struct {
		int error;
	}uvCallBackDataError;

	typedef struct {
		int type;
	}uvCallBackDataDeviceChange;

	typedef struct {
		int size;
		int width;
		int height;
		int type;
		uint8_t *data;
	}uvCallBackDataPreviewYUV;

	typedef struct {
		char src[260];
		int progress;
		int total;
	}uvCallBackDataRemuxProgress;

	typedef struct {
		char src[260];
		int state;
		int error;
	}uvCallBackDataRemuxState;

	typedef struct {
		uvCallbackType type;
		int *data;
	}uvCallBackChunk;

	typedef struct {
		Isolate* isolate;
		Persistent<Context> context;
		Persistent<Function> callback;
		Persistent<Object> object;
	}uvCallBackHandler;

	static uvCallBackHandler* cb_uv_duration = NULL;
	static uvCallBackHandler* cb_uv_error = NULL;
	static uvCallBackHandler* cb_uv_device_change = NULL;
	static uvCallBackHandler* cb_uv_preview_yuv = NULL;
	static std::map<std::string, uvCallBackHandler*> map_uv_remux_progress;
	static std::map<std::string, uvCallBackHandler*> map_uv_remux_state;

	static uv_async_t s_async = { 0 };
	static Locker locker;

	static std::queue<uvCallBackChunk> cb_chunk_queue;

	static void OnUvCallback(uv_async_t *handle);

	static bool isUvInited = false;
	void InitUvEvent() {
		locker.Lock();

		if (isUvInited == false) {
			int ret = uv_async_init(uv_default_loop(), &s_async, OnUvCallback);
			AMLOG("uv init result:%d", ret);

			isUvInited = true;
		}

		locker.Unlock();
	}

	void FreeUvEvent(){
		locker.Lock();

		if (isUvInited == true) {
			uv_close((uv_handle_t*)&s_async, NULL);

			AMLOG("uv freed");

			isUvInited = false;
		}

		locker.Unlock();
	}

	void PushUvChunk(const uvCallBackChunk &chunk) {

		locker.Lock();
		cb_chunk_queue.push(chunk);
		locker.Unlock();

		uv_async_send(&s_async);

	}

	void DispatchUvRecorderDuration(uvCallBackDataDruation *data) {
		if (!cb_uv_duration) return;

		Isolate * isolate = cb_uv_duration->isolate;

		HandleScope scope(isolate);

		const unsigned argc = 1;
		Local<Value> argv[argc] = { Uint32::New(isolate, data->duration) };
		Local<Value> recv;

		cb_uv_duration->callback.Get(isolate)->Call(isolate->GetCurrentContext(), cb_uv_duration->object.Get(isolate), argc, argv);
	}

	void DispatchUvRecorderError(uvCallBackDataError *data) {
		if (!cb_uv_error) return;

		Isolate * isolate = cb_uv_error->isolate;

		HandleScope scope(isolate);

		const unsigned argc = 1;
		Local<Value> argv[argc] = {
			Uint32::New(isolate, data->error)
		};
		Local<Value> recv;

		cb_uv_error->callback.Get(isolate)->Call(isolate->GetCurrentContext(),cb_uv_error->object.Get(isolate), argc, argv);
	}

	void DispatchUvRecorderDeviceChange(uvCallBackDataDeviceChange *data) {
		if (!cb_uv_device_change) return;

		Isolate * isolate = cb_uv_device_change->isolate;

		HandleScope scope(isolate);

		const unsigned argc = 1;
		Local<Value> argv[argc] = {
			Uint32::New(isolate, data->type)
		};
		Local<Value> recv;

		cb_uv_device_change->callback.Get(isolate)->Call(isolate->GetCurrentContext(), cb_uv_device_change->object.Get(isolate), argc, argv);
	}

	void DispatchUvRecorderPreviewYuv(uvCallBackDataPreviewYUV *data) {
		if (!cb_uv_preview_yuv) return;

		Isolate * isolate = cb_uv_preview_yuv->isolate;

		HandleScope scope(isolate);

		const unsigned argc = 5;
		Local<Value> argv[argc] = {
			Uint32::New(isolate, data->size),
			Uint32::New(isolate,data->width),
			Uint32::New(isolate,data->height),
			Uint32::New(isolate,data->type),
			Nan::CopyBuffer((const char *)data->data, data->size).ToLocalChecked()
		};

		Local<Value> recv;

		cb_uv_preview_yuv->callback.Get(isolate)->Call(isolate->GetCurrentContext(), cb_uv_preview_yuv->object.Get(isolate), argc, argv);

	}

	void DispatchUvRecorderRemuxProgress(uvCallBackDataRemuxProgress *data) {
		auto itr = map_uv_remux_progress.find(data->src);
		if (itr == map_uv_remux_progress.end()) return;

		Isolate * isolate = itr->second->isolate;

		HandleScope scope(isolate);

		const unsigned argc = 3;
		Local<Value> argv[argc] = {
			utf8_v8string(isolate, data->src),
			Uint32::New(isolate,data->progress),
			Uint32::New(isolate,data->total),
		};

		Local<Value> recv;

		itr->second->callback.Get(isolate)->Call(isolate->GetCurrentContext(), itr->second->object.Get(isolate), argc, argv);
	}

	void DispatchUvRecorderRemuxState(uvCallBackDataRemuxState *data) {
		auto itr = map_uv_remux_state.find(data->src);
		if (itr == map_uv_remux_state.end()) return;


		Isolate * isolate = itr->second->isolate;

		HandleScope scope(isolate);

		const unsigned argc = 3;
		Local<Value> argv[argc] = {
			utf8_v8string(isolate, data->src),
			Uint32::New(isolate,data->state),
			Uint32::New(isolate,data->error),
		};

		Local<Value> recv;

		itr->second->callback.Get(isolate)->Call(isolate->GetCurrentContext(), itr->second->object.Get(isolate), argc, argv);

		//get state 0 should remove callback functions
		if (data->state == 0) {
			delete itr->second;
			map_uv_remux_state.erase(itr);

			auto itr_progress = map_uv_remux_progress.find(data->src);
			if (itr_progress != map_uv_remux_progress.end()) {
				delete itr_progress->second;
				map_uv_remux_progress.erase(itr_progress);
			}
		}
	}

	void OnRecorderDuration(uint64_t duration) {
		uvCallBackDataDruation *data = new uvCallBackDataDruation;
		data->duration = duration;


		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_duration;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderError(int error) {
		uvCallBackDataError *data = new uvCallBackDataError;
		data->error = error;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_error;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderDeviceChange(int type) {
		uvCallBackDataDeviceChange *data = new uvCallBackDataDeviceChange;
		data->type = type;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_device_change;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderPreviewYuv(const unsigned char *data, unsigned int size, int width, int height,int type) {

		if (!cb_uv_preview_yuv) return;

		char *buff = new char[sizeof(uvCallBackDataPreviewYUV) + size];
		uvCallBackDataPreviewYUV *chunk = (uvCallBackDataPreviewYUV*)buff;

		chunk->data = (uint8_t*)(buff + sizeof(uvCallBackDataPreviewYUV));
		memcpy(chunk->data, data, size);

		chunk->size = size;
		chunk->width = width;
		chunk->height = height;
		chunk->type = type;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_preview_yuv;
		uv_cb_chunk.data = (int*)buff;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderRemuxProgress(const char *src, int progress, int total) {
		uvCallBackDataRemuxProgress *data = new uvCallBackDataRemuxProgress;
		sprintf_s(data->src, 260, "%s", src);
		data->progress = progress;
		data->total = total;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_remux_progress;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderRemuxState(const char *src, int state, int error) {
		uvCallBackDataRemuxState *data = new uvCallBackDataRemuxState;
		sprintf_s(data->src, 260, "%s", src);
		data->state = state;
		data->error = error;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_remux_state;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}


	void OnUvCallback(uv_async_t *handle) {

		locker.Lock();
		while (!cb_chunk_queue.empty()) {
			uvCallBackChunk &chunk = cb_chunk_queue.front();
			cb_chunk_queue.pop();
			switch (chunk.type)
			{
			case uvcb_type_duration:
				DispatchUvRecorderDuration((uvCallBackDataDruation*)chunk.data);
				break;
			case uvcb_type_error:
				DispatchUvRecorderError((uvCallBackDataError*)chunk.data);
				break;
			case uvcb_type_device_change:
				DispatchUvRecorderDeviceChange((uvCallBackDataDeviceChange*)chunk.data);
				break;
			case uvcb_type_preview_yuv:
				DispatchUvRecorderPreviewYuv((uvCallBackDataPreviewYUV*)chunk.data);
				break;
			case uvcb_type_remux_progress:
				DispatchUvRecorderRemuxProgress((uvCallBackDataRemuxProgress*)chunk.data);
				break;
			case uvcb_type_remux_state:
				DispatchUvRecorderRemuxState((uvCallBackDataRemuxState*)chunk.data);
				break;
			default:
				break;
			}

			delete[] chunk.data;
		}

		locker.Unlock();

	}

	void GetSpeakers(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		AMRECORDER_DEVICE *devices;
		int ret = recorder_get_speakers(&devices);
		if (ret > 0) {

			Local<Array> array = Array::New(isolate, ret);

			for (int i = 0; i<ret; i++) {
				Local<Object> device = Object::New(isolate);
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "id"), utf8_v8string(isolate, devices[i].id));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "name"), utf8_v8string(isolate, devices[i].name));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "isDefault"), Number::New(isolate, devices[i].is_default));
				array->Set(isolate->GetCurrentContext(), i, device);
			}

			delete[]devices;

			args.GetReturnValue().Set(array);
		}
		else {
			args.GetReturnValue().Set(utf8_v8string(isolate, "get speaker list failed."));
		}
	}

	void GetMics(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		AMRECORDER_DEVICE *devices;
		int ret = recorder_get_mics(&devices);
		if (ret > 0) {

			Local<Array> array = Array::New(isolate, ret);

			for (int i = 0; i<ret; i++) {
				Local<Object> device = Object::New(isolate);
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "id"), utf8_v8string(isolate, devices[i].id));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "name"), utf8_v8string(isolate, devices[i].name));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "isDefault"), Number::New(isolate, devices[i].is_default));
				array->Set(isolate->GetCurrentContext(), i, device);
			}

			delete[]devices;

			args.GetReturnValue().Set(array);
		}
		else {
			args.GetReturnValue().Set(utf8_v8string(isolate, "get mic list failed."));
		}
	}

	void GetCameras(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		args.GetReturnValue().Set(utf8_v8string(isolate, "get camera list not support for now."));
	}

	void SetDurationCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		locker.Lock();

		if (cb_uv_duration != NULL) {
			delete cb_uv_duration;
			cb_uv_duration = NULL;
		}

		cb_uv_duration = new uvCallBackHandler;
		cb_uv_duration->isolate = isolate;
		cb_uv_duration->context.Reset(isolate, isolate->GetCurrentContext());
		cb_uv_duration->object.Reset(isolate, args.This());
		cb_uv_duration->callback.Reset(isolate, args[0].As<Function>());

		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void SetErrorCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		locker.Lock();

		if (cb_uv_error != NULL) {
			delete cb_uv_error;
			cb_uv_error = NULL;
		}

		cb_uv_error = new uvCallBackHandler;
		cb_uv_error->isolate = isolate;
		cb_uv_error->context.Reset(isolate, isolate->GetCurrentContext());
		cb_uv_error->object.Reset(isolate, args.This());
		cb_uv_error->callback.Reset(isolate, args[0].As<Function>());

		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void SetDeviceChangeCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		locker.Lock();

		if (cb_uv_device_change != NULL) {
			delete cb_uv_device_change;
			cb_uv_device_change = NULL;
		}

		cb_uv_device_change = new uvCallBackHandler;
		cb_uv_device_change->isolate = isolate;
		cb_uv_device_change->context.Reset(isolate, isolate->GetCurrentContext());
		cb_uv_device_change->object.Reset(isolate, args.This());
		cb_uv_device_change->callback.Reset(isolate, args[0].As<Function>());

		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void SetPreviewYuvCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");
		CHECK_PARAM_VALID(args[0], "function");

		locker.Lock();
		if (cb_uv_preview_yuv != NULL) {
			delete cb_uv_preview_yuv;
			cb_uv_preview_yuv = NULL;
		}

		cb_uv_preview_yuv = new uvCallBackHandler;
		cb_uv_preview_yuv->isolate = isolate;
		cb_uv_preview_yuv->context.Reset(isolate, isolate->GetCurrentContext());
		cb_uv_preview_yuv->object.Reset(isolate, args.This());
		cb_uv_preview_yuv->callback.Reset(isolate, args[0].As<Function>());

		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Init(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		//v_qb v_frame_rate v_output a_speaker a_mic
		CHECK_PARAM_COUNT(8);
		CHECK_PARAM_TYPE8("uint32", "uint32", " string", "string", "string", "string", "string", "uint32");

		int error = 0;

		AMRECORDER_SETTING settings;
		AMRECORDER_CALLBACK callbacks;

		callbacks.func_duration = OnRecorderDuration;
		callbacks.func_error = OnRecorderError;
		callbacks.func_device_change = OnRecorderDeviceChange;
		callbacks.func_preview_yuv = OnRecorderPreviewYuv;

		settings.v_left = 0;
		settings.v_top = 0;
		settings.v_width = GetSystemMetrics(SM_CXSCREEN);
		settings.v_height = GetSystemMetrics(SM_CYSCREEN);
		settings.v_bit_rate = 1280 * 1000;

		settings.v_enc_id = args[7]->Uint32Value();
		settings.v_qb = args[0]->Uint32Value();
		settings.v_frame_rate = args[1]->Uint32Value();

		String::Utf8Value utf8Output(Local<String>::Cast(args[2]));
		sprintf_s(settings.output, 260, "%s", *utf8Output);

		String::Utf8Value utf8SpeakerName(Local<String>::Cast(args[3]));
		sprintf_s(settings.a_speaker.name, 260, "%s", *utf8SpeakerName);

		String::Utf8Value utf8SpeakerId(Local<String>::Cast(args[4]));
		sprintf_s(settings.a_speaker.id, 260, "%s", *utf8SpeakerId);

		String::Utf8Value utf8MicName(Local<String>::Cast(args[5]));
		sprintf_s(settings.a_mic.name, 260, "%s", *utf8MicName);

		String::Utf8Value utf8MicId(Local<String>::Cast(args[6]));
		sprintf_s(settings.a_mic.id, 260, "%s", *utf8MicId);

		error = recorder_init(settings, callbacks);

		if (error == 0) InitUvEvent();

		args.GetReturnValue().Set(Int32::New(isolate, error));
	}

	void Release(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_release();

		FreeUvEvent();

		locker.Lock();

		while (!cb_chunk_queue.empty()) {
			uvCallBackChunk &chunk = cb_chunk_queue.front();
			cb_chunk_queue.pop();

			delete[] chunk.data;
		}

		if(cb_uv_duration != NULL){
			delete cb_uv_duration;
			cb_uv_duration = NULL;
		}
		if(cb_uv_error != NULL){
			delete cb_uv_error;
			cb_uv_error = NULL;
		}
		if(cb_uv_device_change != NULL){
			delete cb_uv_device_change;
			cb_uv_device_change = NULL;
		}
		if(cb_uv_preview_yuv != NULL){
			delete cb_uv_preview_yuv;
			cb_uv_preview_yuv = NULL;
		}

		for (auto itr = map_uv_remux_progress.begin(); itr != map_uv_remux_progress.end(); itr++) {
			delete itr->second;
			map_uv_remux_progress.erase(itr);
		}

		for (auto itr = map_uv_remux_state.begin(); itr != map_uv_remux_state.end(); itr++) {
			delete itr->second;
			map_uv_remux_state.erase(itr);
		}

		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Start(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		int error = recorder_start();

		args.GetReturnValue().Set(Int32::New(isolate, error));
	}

	void Stop(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_stop();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Pause(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_pause();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Resume(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_resume();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Wait(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("uint32");

		int timestamp = args[0]->Uint32Value();

		Sleep(timestamp);


		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void GetErrorStr(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("uint32");

		int error_code = args[0]->Uint32Value();

		args.GetReturnValue().Set(utf8_v8string(isolate, recorder_err2str(error_code)));
	}

	void GetVideoEncoders(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		AMRECORDER_ENCODERS *encoders;
		int ret = recorder_get_vencoders(&encoders);
		if (ret > 0) {

			Local<Array> array = Array::New(isolate, ret);

			for (int i = 0; i<ret; i++) {
				Local<Object> encoder = Object::New(isolate);

				encoder->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "name"), utf8_v8string(isolate, encoders[i].name));
				encoder->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "id"), Number::New(isolate, encoders[i].id));

				array->Set(isolate->GetCurrentContext(), i, encoder);
			}

			delete[] encoders;

			args.GetReturnValue().Set(array);
		}
		else {
			args.GetReturnValue().Set(utf8_v8string(isolate, "get video encoder list failed."));
		}
	}

	void RemuxFile(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		CHECK_PARAM_COUNT(4);
		CHECK_PARAM_TYPE4("string", "string", "function", "function");

		//in case that user hav not call InitUvEvent before
		InitUvEvent();

		char src[260] = { 0 }, dst[260] = { 0 };

		String::Utf8Value v8Src(Local<String>::Cast(args[0]));
		sprintf_s(src, 260, "%s", *v8Src);

		String::Utf8Value v8Dst(Local<String>::Cast(args[1]));
		sprintf_s(dst, 260, "%s", *v8Dst);

		locker.Lock();

		auto itr_progress = map_uv_remux_progress.find(src);
		if (itr_progress != map_uv_remux_progress.end()) {
			map_uv_remux_progress.erase(itr_progress);
		}

		auto itr_state = map_uv_remux_state.find(src);
		if (itr_state != map_uv_remux_state.end()) {
			map_uv_remux_state.erase(itr_state);
		}

		
		uvCallBackHandler *uvProgressHandler = new uvCallBackHandler;

		uvProgressHandler->isolate = isolate;
		uvProgressHandler->context.Reset(isolate, isolate->GetCurrentContext());
		uvProgressHandler->object.Reset(isolate, args.This());
		uvProgressHandler->callback.Reset(isolate, args[2].As<Function>());
		map_uv_remux_progress[src] = uvProgressHandler;


		uvCallBackHandler *uvStateHandler = new uvCallBackHandler;

		uvStateHandler->isolate = isolate;
		uvStateHandler->context.Reset(isolate, isolate->GetCurrentContext());
		uvStateHandler->object.Reset(isolate, args.This());
		uvStateHandler->callback.Reset(isolate, args[3].As<Function>());
		map_uv_remux_state[src] = uvStateHandler;
		

		locker.Unlock();

		int ret = recorder_remux(src, dst, OnRecorderRemuxProgress, OnRecorderRemuxState);


		args.GetReturnValue().Set(ret);
	}

	void Initialize(Local<Object> exports)
	{
		NODE_SET_METHOD(exports, "GetSpeakers", GetSpeakers);
		NODE_SET_METHOD(exports, "GetMics", GetMics);
		NODE_SET_METHOD(exports, "GetCameras", GetCameras);
		NODE_SET_METHOD(exports, "SetDurationCallBack", SetDurationCallBack);
		NODE_SET_METHOD(exports, "SetDeviceChangeCallBack", SetDeviceChangeCallBack);
		NODE_SET_METHOD(exports, "SetErrorCallBack", SetErrorCallBack);
		NODE_SET_METHOD(exports, "SetPreviewYuvCallBack", SetPreviewYuvCallBack);
		NODE_SET_METHOD(exports, "Init", Init);
		NODE_SET_METHOD(exports, "Release", Release);
		NODE_SET_METHOD(exports, "Start", Start);
		NODE_SET_METHOD(exports, "Stop", Stop);
		NODE_SET_METHOD(exports, "Pause", Pause);
		NODE_SET_METHOD(exports, "Resume", Resume);
		NODE_SET_METHOD(exports, "Wait", Wait);
		NODE_SET_METHOD(exports, "GetErrorStr", GetErrorStr);
		NODE_SET_METHOD(exports, "GetVideoEncoders", GetVideoEncoders);
		NODE_SET_METHOD(exports, "RemuxFile", RemuxFile);
	}

	NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

} // namespace recorder
