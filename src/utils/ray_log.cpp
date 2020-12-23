#include "ray_log.h"

#include <sys/timeb.h>
#include <algorithm>
#include <ctime>
#include <iomanip>

#include <io.h>
#include <windows.h>

#include "ray_string.h"

namespace ray {
namespace utils {
typedef HANDLE FileHandle;
typedef HANDLE MutexHandle;

namespace {

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
	"INFO","WARNING", "ERROR", "FATAL" };

int min_log_level = 0;

std::wstring* log_file_name = NULL;

// this file is lazily opened and the handle may be NULL
FileHandle log_file = NULL;


int32_t CurrentProcessId() {
	return GetCurrentProcessId();
}

uint64_t TickCount() {
	return GetTickCount();
}

void CloseFile(FileHandle log) {
	CloseHandle(log);
}

void DeleteFilePath(const std::wstring& log_name) {
	DeleteFile(log_name.c_str());
}

std::wstring GetDefaultLogFile() {
	// On Windows we use the same path as the exe.
	wchar_t module_name[MAX_PATH];
	GetModuleFileName(NULL, module_name, MAX_PATH);

	std::wstring log_file = module_name;
	std::wstring::size_type last_backslash =
		log_file.rfind('\\', log_file.size());
	if (last_backslash != std::wstring::npos)
		log_file.erase(last_backslash + 1);
	log_file += L"debug.log";
	return log_file;
}

// This class acts as a wrapper for locking the logging files.
// LoggingLock::Init() should be called from the main thread before any logging
// is done. Then whenever logging, be sure to have a local LoggingLock
// instance on the stack. This will ensure that the lock is unlocked upon
// exiting the frame.
// LoggingLocks can not be nested.
class LoggingLock {
public:
	LoggingLock() {
		LockLogging();
	}

	~LoggingLock() {
		UnlockLogging();
	}

	static void Init(const wchar_t* new_log_file) {
		if (initialized)
			return;

		if (!log_mutex) {
			std::wstring safe_name;
			if (new_log_file)
				safe_name = new_log_file;
			else
				safe_name = GetDefaultLogFile();
			// \ is not a legal character in mutex names so we replace \ with /
			std::replace(safe_name.begin(), safe_name.end(), '\\', '/');
			std::wstring t(L"Global\\");
			t.append(safe_name);
			log_mutex = ::CreateMutex(NULL, FALSE, t.c_str());

			if (log_mutex == NULL) {
				return;
			}
		}

		initialized = true;
	}

private:
	static void LockLogging() {
		::WaitForSingleObject(log_mutex, INFINITE);
	}

	static void UnlockLogging() {
		ReleaseMutex(log_mutex);
	}

	// When we don't use a lock, we are using a global mutex. We need to do this
	// because LockFileEx is not thread safe.
	static MutexHandle log_mutex;

	static bool initialized;
};

// static
bool LoggingLock::initialized = false;

// static
MutexHandle LoggingLock::log_mutex = NULL;

// Called by logging functions to ensure that debug_file is initialized
// and can be used for writing. Returns false if the file could not be
// initialized. debug_file will be NULL in this case.
bool InitializeLogFileHandle() {
	if (log_file)
		return true;

	if (!log_file_name || !log_file_name->length()) {
		if (log_file_name) delete log_file_name;
		// Nobody has called InitLogging to specify a debug log file, so here we
		// initialize the log file name to a default.
		log_file_name = new std::wstring(GetDefaultLogFile());
	}

	log_file = CreateFile(log_file_name->c_str(), GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (log_file == INVALID_HANDLE_VALUE || log_file == NULL) {
		// try the current directory
		log_file = CreateFile(L".\\debug.log", GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (log_file == INVALID_HANDLE_VALUE || log_file == NULL) {
			log_file = NULL;
			return false;
		}
	}
	SetFilePointer(log_file, 0, 0, FILE_END);

	return true;
}

}  // namespace


bool InitLogImpl(const wchar_t* new_log_file, LOG_LEVEL min_level) {


	LoggingLock::Init(new_log_file);

	LoggingLock logging_lock;

	if (log_file) {
		// calling InitLogging twice or after some log call has already opened the
		// default log file will re-initialize to the new options
		CloseFile(log_file);
		log_file = NULL;
	}

	if (!log_file_name)
		log_file_name = new std::wstring();

	if (new_log_file)
		*log_file_name = new_log_file;

	// DeleteFilePath(*log_file_name);

	SetMinLogLevel(min_level);

	return InitializeLogFileHandle();
}

void SetMinLogLevel(int level) {
	min_log_level = min(LOG_ERROR, level);
}

int GetMinLogLevel() {
	return min_log_level;
}

LogOnce::LogOnce(LOG_LEVEL level)
	: level_(level) {
	Init(nullptr, -1);
}

LogOnce::LogOnce(const char* file, int line, LOG_LEVEL level)
	: level_(level) {
	Init(file, line);
}

LogOnce::~LogOnce() {
	stream_ << std::endl;
	std::string str_newline(stream_.str());

	OutputDebugStringA(str_newline.c_str());

	fprintf(stderr, "%s", str_newline.c_str());
	fflush(stderr);

	// We can have multiple threads and/or processes, so try to prevent them
	// from clobbering each other's writes.
	// If the client app did not call InitLogging, and the lock has not
	// been created do it now. We do this on demand, but if two threads try
	// to do this at the same time, there will be a race condition to create
	// the lock. This is why InitLogging should be called from the main
	// thread at the beginning of execution.
	LoggingLock::Init(nullptr);

	// write to log file
	LoggingLock logging_lock;
	if (InitializeLogFileHandle()) {
		SetFilePointer(log_file, 0, 0, SEEK_END);
		DWORD num_written;
		WriteFile(log_file,
			static_cast<const void*>(str_newline.c_str()),
			static_cast<DWORD>(str_newline.length()),
			&num_written,
			NULL);
	}
}

// writes the common header info to the stream
void LogOnce::Init(const char* file, int line) {

	std::string filename;
	if (file) {
		filename = file;
		size_t last_slash_pos = filename.find_last_of("\\/");
		if (last_slash_pos != std::string::npos)
			filename = filename.substr(last_slash_pos + 1);
	}

	SYSTEMTIME now;
	GetLocalTime(&now);

	struct timeb tp_cur;
	ftime(&tp_cur);

	stream_ << std::setfill('0')
		<< std::setw(4) << now.wYear << "-"
		<< std::setw(2) << now.wMonth << "-"
		<< std::setw(2) << now.wDay << "-"
		<< 'T'
		<< std::setw(2) << now.wHour << ":"
		<< std::setw(2) << now.wMinute << ":"
		<< std::setw(2) << now.wSecond << ":"
		<< std::setw(4) << now.wMilliseconds << " "

		// GMT +0800 (+08:00)
		// tp_cur.timezone = gmt - local
		// so hour is abs(tp_cur.timezone / 60)
		// minute is abs(tp_cur.timezone) - abs(tp_cur.timezone / 60) *60
		<< (tp_cur.timezone > 0 ? "-" : "+")
		<< std::setw(2) << abs(tp_cur.timezone / 60)
		<< std::setw(2) << abs(tp_cur.timezone) - abs(tp_cur.timezone / 60) * 60
		<< " ";


	if (level_ >= 0)
		stream_ << "[" << log_severity_names[level_] << "]";
	else
		stream_ << "[" << "VERBOSE-" << -level_ << "]";

	if (filename.length() && line != -1) {
		stream_ << " [" << filename.c_str() << "(" << line << ")]";
	}

	stream_ << " ";
}

void ReleaseLogImpl() {
	LoggingLock logging_lock;

	if (!log_file)
		return;

	CloseFile(log_file);
	log_file = NULL;
}


std::wstring GetLogFileFullPath() {
	if (log_file_name)
		return *log_file_name;
	return std::wstring();
}
}
}

std::ostream& operator<<(std::ostream& out, const wchar_t* str) {
	if (str)
		return out << ray::utils::strings::unicode_utf8(str);
	else
		return out;
}
