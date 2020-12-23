#pragma once

#include <string>
#include <sstream>
#include <iostream>

namespace ray {
namespace utils {

typedef int LOG_LEVEL;
const LOG_LEVEL LOG_VERBOSE = -1;
const LOG_LEVEL LOG_INFO = 0;
const LOG_LEVEL LOG_WARNING = 1;
const LOG_LEVEL LOG_ERROR = 2;
const LOG_LEVEL LOG_FATAL = 3;
const LOG_LEVEL LOG_NUM_SEVERITIES = 4;

// This class is used to explicitly ignore values in the conditional
// log macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogOnceVoidify {
public:
	LogOnceVoidify() { }
	// This has to be an operator with a precedence lower than << but
	// higher than ?:
	void operator&(std::ostream&) { }
};

class LogOnce {
public:
	LogOnce(LOG_LEVEL level);

	LogOnce(const char* file, int line, LOG_LEVEL level);

	~LogOnce();

	std::ostream& stream() { return stream_; }

private:
	void Init(const char* file, int line);

	LOG_LEVEL level_;
	std::ostringstream stream_;
};

/**
* Sets the log file name and other global logging state.
* Calling this function is recommended, and is normally
* done at the beginning of application init.
* @param log_file log file path
* @param min_level minimum log level
* @return true for succeed,otherwise failed
*/
bool InitLogImpl(const wchar_t* log_file, LOG_LEVEL min_level = LOG_INFO);

/**
* Close log file
*/
void ReleaseLogImpl();

/**
* Set minimum log level
*/
void SetMinLogLevel(int level);

/**
* Get minimum log level
*/
int GetMinLogLevel();

/**
* Get full log file path
*/
std::wstring GetLogFileFullPath();

}
}

#ifdef _DEBUG

#define LOG_STREAM_LOG_INFO \
    ray::utils::LogOnce(__FILE__, __LINE__, ray::utils::LOG_INFO)

#define LOG_STREAM_LOG_WARNING \
    ray::utils::LogOnce(__FILE__, __LINE__, ray::utils::LOG_WARNING)

#define LOG_STREAM_LOG_ERROR \
    ray::utils::LogOnce(__FILE__, __LINE__, ray::utils::LOG_ERROR)

#define LOG_STREAM_LOG_FATAL \
    ray::utils::LogOnce(__FILE__, __LINE__, ray::utils::LOG_FATAL)

#else

#define LOG_STREAM_LOG_INFO \
    ray::utils::LogOnce(ray::utils::LOG_INFO)

#define LOG_STREAM_LOG_WARNING \
    ray::utils::LogOnce(ray::utils::LOG_WARNING)

#define LOG_STREAM_LOG_ERROR \
    ray::utils::LogOnce(ray::utils::LOG_ERROR)

#define LOG_STREAM_LOG_FATAL \
    ray::utils::LogOnce(ray::utils::LOG_FATAL)

#endif // _DEBUG

#define LOG_IS_ON(level) \
    ((ray::utils::LOG_ ## level) >= ray::utils::GetMinLogLevel())

#define LOG_STREAM(level) LOG_STREAM_LOG_ ## level.stream()

#define LOG_LAZY_STREAM(stream, condition) \
    !(condition) ? (void) 0 : ray::utils::LogOnceVoidify() & (stream)

#define LOG(level) LOG_LAZY_STREAM(LOG_STREAM(level), LOG_IS_ON(level))

#define LOG_IF(level, condition) \
    LOG_LAZY_STREAM(LOG_STREAM(level), LOG_IS_ON(level) && (condition))

#define VLOG_STREAM(verbose_level) \
    ray::utils::LogOnce(__FILE__, __LINE__, -verbose_level).stream()

#define VLOG_IS_ON(verbose_level) \
    ((-verbose_level) <= ray::utils::GetMinLogLevel() && ray::utils::GetMinLogLevel() <= ray::utils::LOG_VERBOSE)

#define VLOG(verbose_level) \
    LOG_LAZY_STREAM(VLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level)) << L" "

#define VLOG_IF(verbose_level,condition) \
    LOG_LAZY_STREAM(VLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level) && (condition))

#define INFO INFO
#define WARNING WARNING
#define ERROR ERROR
#define FATAL FATAL

#define VLOG_DEBUG 1
#define VLOG_TEST 2

#define ENUM2STR(index,strarray) strarray[index]

std::ostream& operator<<(std::ostream& out, const wchar_t* str);

inline std::ostream& operator<<(std::ostream& out, const std::wstring& str) {
	return out << str.c_str();
}
