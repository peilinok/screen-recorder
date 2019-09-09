#ifndef ERROR_DEFINE
#define ERROR_DEFINE


enum AM_ERROR{
	AE_NO = 0,
	AE_UNSUPPORT,
	AE_INVALID_CONTEXT,
	AE_NEED_INIT,

	AE_CO_INITED_FAILED,
	AE_CO_CREATE_FAILED,
	AE_CO_GETENDPOINT_FAILED,
	AE_CO_ACTIVE_DEVICE_FAILED,
	AE_CO_GET_FORMAT_FAILED,
	AE_CO_AUDIOCLIENT_INIT_FAILED,
	AE_CO_GET_CAPTURE_FAILED,
	AE_CO_CREATE_EVENT_FAILED,
	AE_CO_SET_EVENT_FAILED,
	AE_CO_START_FAILED,
};

static const char *ERRORS_STR[] = {
	"no error",                      //AE_NO
	"not support for now",           //AE_UNSUPPORT
	"invalid context",               //AE_INVALID_CONTEXT
	"need init first",               //AE_NEED_INIT

	"com init failed",               //AE_CO_INITED_FAILED
	"com create instance failed",    //AE_CO_CREATE_FAILED
	"com get endpoint failed",       //AE_CO_GETENDPOINT_FAILED
	"com active device failed",      //AE_CO_ACTIVE_DEVICE_FAILED
	"com get wave formatex failed",  //AE_CO_GET_FORMAT_FAILED
	"com audio client init failed",  //AE_CO_AUDIOCLIENT_INIT_FAILED
	"com audio get capture failed",  //AE_CO_GET_CAPTURE_FAILED
	"com audio create event failed", //AE_CO_CREATE_EVENT_FAILED
	"com set ready event failed",    //AE_CO_SET_EVENT_FAILED
	"com start to record failed",    //AE_CO_START_FAILED

};

#define err2str(e) ERRORS_STR[e]

#endif // !ERROR_DEFINE