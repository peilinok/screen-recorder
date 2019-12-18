#ifndef ERROR_DEFINE
#define ERROR_DEFINE


enum AM_ERROR{
	AE_NO = 0,
	AE_ERROR,
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
	AE_CO_ENUMENDPOINT_FAILED,
	AE_CO_GET_ENDPOINT_COUNT_FAILED,
	AE_CO_GET_ENDPOINT_ID_FAILED,
	AE_CO_OPEN_PROPERTY_FAILED,
	AE_CO_GET_VALUE_FAILED,
	AE_CO_GET_BUFFER_FAILED,
	AE_CO_RELEASE_BUFFER_FAILED,
	AE_CO_GET_PACKET_FAILED,
	AE_CO_PADDING_UNEXPECTED,

	AE_FFMPEG_OPEN_INPUT_FAILED,
	AE_FFMPEG_FIND_STREAM_FAILED,
	AE_FFMPEG_FIND_DECODER_FAILED,
	AE_FFMPEG_OPEN_CODEC_FAILED,
	AE_FFMPEG_READ_FRAME_FAILED,
	AE_FFMPEG_READ_PACKET_FAILED,
	AE_FFMPEG_DECODE_FRAME_FAILED,
	AE_FFMPEG_NEW_SWSCALE_FAILED,
	AE_FFMPEG_FIND_ENCODER_FAILED,
	AE_FFMPEG_ALLOC_CONTEXT_FAILED,
	AE_FFMPEG_ENCODE_FRAME_FAILED,
	AE_FFMPEG_ALLOC_FRAME_FAILED,
	AE_FFMPEG_OPEN_IO_FAILED,
	AE_FFMPEG_CREATE_STREAM_FAILED,
	AE_FFMPEG_COPY_PARAMS_FAILED,
	AE_RESAMPLE_INIT_FAILED,
	AE_FFMPEG_NEW_STREAM_FAILED,
	AE_FFMPEG_FIND_INPUT_FMT_FAILED,

	AE_MP4V2_CREATE_FAILED,
	AE_MP4V2_ADD_TRACK_FAILED,

	AE_FILTER_ALLOC_GRAPH_FAILED,
	AE_FILTER_CREATE_FILTER_FAILED,
	AE_FILTER_PARSE_PTR_FAILED,
	AE_FILTER_CONFIG_FAILED,
	AE_FILTER_INVALID_CTX_INDEX,
	AE_FILTER_ADD_FRAME_FAILED,

	AE_GDI_GET_DC_FAILED,
	AE_GDI_CREATE_DC_FAILED,
	AE_GDI_CREATE_BMP_FAILED,
	AE_GDI_BITBLT_FAILED,
	AE_GDI_GET_DIBITS_FAILED,
};

static const char *ERRORS_STR[] = {
	"no error",                         //AE_NO
	"error",                            //AE_ERROR
	"not support for now",              //AE_UNSUPPORT
	"invalid context",                  //AE_INVALID_CONTEXT
	"need init first",                  //AE_NEED_INIT

	"com init failed",                  //AE_CO_INITED_FAILED
	"com create instance failed",       //AE_CO_CREATE_FAILED
	"com get endpoint failed",          //AE_CO_GETENDPOINT_FAILED
	"com active device failed",         //AE_CO_ACTIVE_DEVICE_FAILED
	"com get wave formatex failed",     //AE_CO_GET_FORMAT_FAILED
	"com audio client init failed",     //AE_CO_AUDIOCLIENT_INIT_FAILED
	"com audio get capture failed",     //AE_CO_GET_CAPTURE_FAILED
	"com audio create event failed",    //AE_CO_CREATE_EVENT_FAILED
	"com set ready event failed",       //AE_CO_SET_EVENT_FAILED
	"com start to record failed",       //AE_CO_START_FAILED
	"com enum audio endpoints failed",  //AE_CO_ENUMENDPOINT_FAILED
	"com get endpoints count failed",   //AE_CO_GET_ENDPOINT_COUNT_FAILED
	"com get endpoint id failed",       //AE_CO_GET_ENDPOINT_ID_FAILED
	"com open endpoint property failed", //AE_CO_OPEN_PROPERTY_FAILED
	"com get property value failed",    //AE_CO_GET_VALUE_FAILED
	"com get buffer failed",            //AE_CO_GET_BUFFER_FAILED
	"com release buffer failed",        //AE_CO_RELEASE_BUFFER_FAILED
	"com get packet size failed",       //AE_CO_GET_PACKET_FAILED
	"com get padding size unexpected",  //AE_CO_PADDING_UNEXPECTED

	"ffmpeg open input failed",         //AE_FFMPEG_OPEN_INPUT_FAILED
	"ffmpeg find stream info failed",   //AE_FFMPEG_FIND_STREAM_FAILED
	"ffmpeg find decoder failed",       //AE_FFMPEG_FIND_DECODER_FAILED
	"ffmpeg open codec failed",         //AE_FFMPEG_OPEN_CODEC_FAILED
	"ffmpeg read frame failed",         //AE_FFMPEG_READ_FRAME_FAILED
	"ffmpeg read packet failed",        //AE_FFMPEG_READ_PACKET_FAILED
	"ffmpeg decode frame failed",       //AE_FFMPEG_DECODE_FRAME_FAILED
	"ffmpeg create swscale failed",     //AE_FFMPEG_NEW_SWSCALE_FAILED

	"ffmpeg find encoder failed",       //AE_FFMPEG_FIND_ENCODER_FAILED
	"ffmpeg alloc context failed",      //AE_FFMPEG_ALLOC_CONTEXT_FAILED
	"ffmpeg encode frame failed",       //AE_FFMPEG_ENCODE_FRAME_FAILED
	"ffmpeg alloc frame failed",        //AE_FFMPEG_ALLOC_FRAME_FAILED
	
	"ffmpeg open io ctx failed",        //AE_FFMPEG_OPEN_IO_FAILED
	"ffmpeg new stream failed",         //AE_FFMPEG_CREATE_STREAM_FAILED
	"ffmpeg copy parameters failed",    //AE_FFMPEG_COPY_PARAMS_FAILED
	"resampler init failed",            //AE_RESAMPLE_INIT_FAILED
	"ffmpeg new out stream failed",     //AE_FFMPEG_NEW_STREAM_FAILED
	"ffmpeg find input format failed",  //AE_FFMPEG_FIND_INPUT_FMT_FAILED

	"mp4v2 create new handle failed",   //AE_MP4V2_CREATE_FAILED
	"mp4v2 create new track failed",    //AE_MP4V2_ADD_TRACK_FAILED

	"avfilter alloc avfilter failed",   //AE_FILTER_ALLOC_GRAPH_FAILED
	"avfilter create graph failed",     //AE_FILTER_CREATE_FILTER_FAILED
	"avfilter parse ptr failed",        //AE_FILTER_PARSE_PTR_FAILED
	"avfilter config graph failed",     //AE_FILTER_CONFIG_FAILED
	"avfilter invalid ctx index",       //AE_FILTER_INVALID_CTX_INDEX
	"avfilter add frame failed",        //AE_FILTER_ADD_FRAME_FAILED

	"gdi get dc failed",                //AE_GDI_GET_DC_FAILED
	"gdi create dc failed",             //AE_GDI_CREATE_DC_FAILED
	"gdi create bmp failed",            //AE_GDI_CREATE_BMP_FAILED
	"gdi bitblt failed",                //AE_GDI_BITBLT_FAILED
	"gid geet dibbits failed",          //AE_GDI_GET_DIBITS_FAILED
};

#define err2str(e) ERRORS_STR[e]

#define AMERROR_CHECK(err) if(err != AE_NO) return err

#endif // !ERROR_DEFINE