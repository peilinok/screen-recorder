#include "ray_dump.h"

#include <Windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <tchar.h>

#if _MSC_VER >= 1300    // for VC 7.0
// from ATL 7.0 sources
#ifndef _delayimp_h
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif
#endif

static void dump_file(const TCHAR *path, EXCEPTION_POINTERS *exception)
{
	HANDLE file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	MINIDUMP_EXCEPTION_INFORMATION dump;
	dump.ExceptionPointers = exception;
	dump.ThreadId = GetCurrentThreadId();
	dump.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithFullMemory, &dump, NULL, NULL);

	CloseHandle(file);
}



static HMODULE get_current_module()
{
#if _MSC_VER < 1300    // earlier than .NET compiler (VC 6.0)

	// Here's a trick that will get you the handle of the module
	// you're running in without any a-priori knowledge:
	// http://www.dotnet247.com/247reference/msgs/13/65259.aspx

	MEMORY_BASIC_INFORMATION mbi;
	static int dummy;
	VirtualQuery(&dummy, &mbi, sizeof(mbi));

	return reinterpret_cast<HMODULE>(mbi.AllocationBase);

#else    // VC 7.0

	// from ATL 7.0 sources

	return reinterpret_cast<HMODULE>(&__ImageBase);
#endif
}

static long exception_handler(EXCEPTION_POINTERS *ep)
{
	TCHAR dmp_path[MAX_PATH] = { 0 };
	TCHAR temp_path[MAX_PATH] = { 0 };

	//c://users//appdata//local//temp//recorder.dmp
	if (GetTempPath(MAX_PATH, temp_path)) {
		swprintf_s(dmp_path, MAX_PATH, L"%srecorder.dmp", temp_path);

		wprintf(L"%s\r\n", dmp_path);
	}


	dump_file(dmp_path, ep);

	return EXCEPTION_EXECUTE_HANDLER;
}

void registerExceptionHandler()
{
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)exception_handler);
}