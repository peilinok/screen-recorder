#pragma once

static const char *ErrorStr[] = {
	"no error",                         //ERR_NO
	"error",                            //ERR_ERROR
	"not support for now",              //ERR_UNSUPPORT
	"invalid context",                  //ERR_INVALID_CONTEXT
	"need init first",                  //ERR_NEED_INIT
	"operation timeout",                //ERR_TIMEOUT
	"allocate memory failed",           //ERR_ALLOCATE_FAILED,

	"com init failed",                  //ERR_CO_INITED_FAILED
	"com create instance failed",       //ERR_CO_CREATE_FAILED
	"com get endpoint failed",          //ERR_CO_GETENDPOINT_FAILED
	"com active device failed",         //ERR_CO_ACTIVE_DEVICE_FAILED
	"com get wave formatex failed",     //ERR_CO_GET_FORMAT_FAILED
	"com audio client init failed",     //ERR_CO_AUDIOCLIENT_INIT_FAILED
	"com audio get capture failed",     //ERR_CO_GET_CAPTURE_FAILED
	"com audio create event failed",    //ERR_CO_CREATE_EVENT_FAILED
	"com set ready event failed",       //ERR_CO_SET_EVENT_FAILED
	"com start to record failed",       //ERR_CO_START_FAILED
	"com enum audio endpoints failed",  //ERR_CO_ENUMENDPOINT_FAILED
	"com get endpoints count failed",   //ERR_CO_GET_ENDPOINT_COUNT_FAILED
	"com get endpoint id failed",       //ERR_CO_GET_ENDPOINT_ID_FAILED
	"com open endpoint property failed", //ERR_CO_OPEN_PROPERTY_FAILED
	"com get property value failed",    //ERR_CO_GET_VALUE_FAILED
	"com get buffer failed",            //ERR_CO_GET_BUFFER_FAILED
	"com release buffer failed",        //ERR_CO_RELEASE_BUFFER_FAILED
	"com get packet size failed",       //ERR_CO_GET_PACKET_FAILED
	"com get padding size unexpected",  //ERR_CO_PADDING_UNEXPECTED

	"ffmpeg open input failed",         //ERR_FFMPEG_OPEN_INPUT_FAILED
	"ffmpeg find stream info failed",   //ERR_FFMPEG_FIND_STREAM_FAILED
	"ffmpeg find decoder failed",       //ERR_FFMPEG_FIND_DECODER_FAILED
	"ffmpeg open codec failed",         //ERR_FFMPEG_OPEN_CODEC_FAILED
	"ffmpeg read frame failed",         //ERR_FFMPEG_READ_FRAME_FAILED
	"ffmpeg read packet failed",        //ERR_FFMPEG_READ_PACKET_FAILED
	"ffmpeg decode frame failed",       //ERR_FFMPEG_DECODE_FRAME_FAILED
	"ffmpeg create swscale failed",     //ERR_FFMPEG_NEW_SWSCALE_FAILED

	"ffmpeg find encoder failed",       //ERR_FFMPEG_FIND_ENCODER_FAILED
	"ffmpeg alloc context failed",      //ERR_FFMPEG_ALLOC_CONTEXT_FAILED
	"ffmpeg encode frame failed",       //ERR_FFMPEG_ENCODE_FRAME_FAILED
	"ffmpeg alloc frame failed",        //ERR_FFMPEG_ALLOC_FRAME_FAILED

	"ffmpeg open io ctx failed",        //ERR_FFMPEG_OPEN_IO_FAILED
	"ffmpeg new stream failed",         //ERR_FFMPEG_CREATE_STREAM_FAILED
	"ffmpeg copy parameters failed",    //ERR_FFMPEG_COPY_PARAMS_FAILED
	"resampler init failed",            //ERR_RESAMPLE_INIT_FAILED
	"ffmpeg new out stream failed",     //ERR_FFMPEG_NEW_STREAM_FAILED
	"ffmpeg find input format failed",  //ERR_FFMPEG_FIND_INPUT_FMT_FAILED
	"ffmpeg write file header failed",  //ERR_FFMPEG_WRITE_HEADER_FAILED
	"ffmpeg write file trailer failed", //ERR_FFMPEG_WRITE_TRAILER_FAILED
	"ffmpeg write frame failed",        //ERR_FFMPEG_WRITE_FRAME_FAILED

	"avfilter alloc avfilter failed",   //ERR_FILTER_ALLOC_GRAPH_FAILED
	"avfilter create graph failed",     //ERR_FILTER_CREATE_FILTER_FAILED
	"avfilter parse ptr failed",        //ERR_FILTER_PARSE_PTR_FAILED
	"avfilter config graph failed",     //ERR_FILTER_CONFIG_FAILED
	"avfilter invalid ctx index",       //ERR_FILTER_INVALID_CTX_INDEX
	"avfilter add frame failed",        //ERR_FILTER_ADD_FRAME_FAILED

	"gdi get dc failed",                //ERR_GDI_GET_DC_FAILED
	"gdi create dc failed",             //ERR_GDI_CREATE_DC_FAILED
	"gdi create bmp failed",            //ERR_GDI_CREATE_BMP_FAILED
	"gdi bitblt failed",                //ERR_GDI_BITBLT_FAILED
	"gid geet dibbits failed",          //ERR_GDI_GET_DIBITS_FAILED

	"d3d11 library load failed",        //ERR_D3D_LOAD_FAILED
	"d3d11 proc get failed",            //ERR_D3D_GET_PROC_FAILED
	"d3d11 create device failed",       //ERR_D3D_CREATE_DEVICE_FAILED
	"d3d11 query interface failed",     //ERR_D3D_QUERYINTERFACE_FAILED
	"d3d11 create vertex shader failed",//ERR_D3D_CREATE_VERTEX_SHADER_FAILED
	"d3d11 create input layout failed", //ERR_D3D_CREATE_INLAYOUT_FAILED
	"d3d11 create pixel shader failed", //ERR_D3D_CREATE_PIXEL_SHADER_FAILED
	"d3d11 create sampler state failed",//ERR_D3D_CREATE_SAMPLERSTATE_FAILED

	"dxgi get proc address failed",     //ERR_DXGI_GET_PROC_FAILED
	"dxgi get adapter failed",          //ERR_DXGI_GET_ADAPTER_FAILED
	"dxgi get factory failed",          //ERR_DXGI_GET_FACTORY_FAILED
	"dxgi specified adapter not found", //ERR_DXGI_FOUND_ADAPTER_FAILED

	"duplication attatch desktop failed", //ERR_DUP_ATTATCH_FAILED
	"duplication query interface failed", //ERR_DUP_QI_FAILED
	"duplication get parent failed",      //ERR_DUP_GET_PARENT_FAILED
	"duplication enum ouput failed",      //ERR_DUP_ENUM_OUTPUT_FAILED
	"duplication duplicate unavailable",  //ERR_DUP_DUPLICATE_MAX_FAILED
	"duplication duplicate failed",       //ERR_DUP_DUPLICATE_FAILED
	"duplication release frame failed",   //ERR_DUP_RELEASE_FRAME_FAILED
	"duplication acquire frame failed",   //ERR_DUP_ACQUIRE_FRAME_FAILED
	"duplication qi frame failed",        //ERR_DUP_QI_FRAME_FAILED
	"duplication create texture failed",  //ERR_DUP_CREATE_TEXTURE_FAILED
	"duplication dxgi qi failed",         //ERR_DUP_QI_DXGI_FAILED
	"duplication map rects failed",       //ERR_DUP_MAP_FAILED
	"duplication get cursor shape failed",//ERR_DUP_GET_CURSORSHAPE_FAILED

	"remux is already running",           //ERR_REMUX_RUNNING
	"remux input file do not exist",      //ERR_REMUX_NOT_EXIST
	"remux input or output file invalid", //ERR_REMUX_INVALID_INOUT
};

#define err2str(e) e < ERR_MAX ? ErrorStr[e] : "unknown"

