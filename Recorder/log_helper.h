#ifndef AM_LOG
#define AM_LOG

#include <stdio.h>
#include <time.h>
#include <sys\timeb.h>

#include <windows.h>

enum AM_LOG_TYPE {
	AL_TYPE_DEBUG = 0,
	AL_TYPE_INFO,
	AL_TYPE_WARN,
	AL_TYPE_ERROR,
	AL_TYPE_FATAL,
};

static const char *AM_LOG_STR[] = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

#define al_printf(type,format,datetime,ms,...)                                 \
        printf("%lu:%s::%s,%.3d::%s::" format "\n",GetCurrentThreadId(),type,datetime,ms,__FUNCTION__,## __VA_ARGS__)

#define al_log(type,format,...) do{                                            \
  struct _timeb now;                                                           \
  struct tm today;                                                             \
  char datetime_str[20];                                                       \
  _ftime_s(&now);                                                              \
  localtime_s(&today, &now.time);                                              \
  strftime(datetime_str, 20, "%Y-%m-%d %H:%M:%S", &today);                     \
  al_printf(AM_LOG_STR[type],format,datetime_str,now.millitm,## __VA_ARGS__);  \
}while (0)                                       

#define al_debug(format, ...) al_log(AL_TYPE_DEBUG, format, ## __VA_ARGS__)
#define al_info(format, ...) al_log(AL_TYPE_INFO, format, ## __VA_ARGS__)
#define al_warn(format, ...) al_log(AL_TYPE_WARN, format, ## __VA_ARGS__)
#define al_error(format, ...) al_log(AL_TYPE_ERROR, format, ## __VA_ARGS__)
#define al_fatal(format, ...) al_log(AL_TYPE_FATAL, format, ## __VA_ARGS__)

#endif