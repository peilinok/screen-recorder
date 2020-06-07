#include <Windows.h>

#include <dbghelp.h>
#include <stdio.h>
#include <tchar.h>

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

#if _MSC_VER >= 1300    // for VC 7.0
// from ATL 7.0 sources
#ifndef _delayimp_h
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif
#endif

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
	char dmp_path[MAX_PATH] = { 0 };
	char temp_path[MAX_PATH] = { 0 };

	if (GetTempPath(MAX_PATH, temp_path)) {
		sprintf_s(dmp_path, MAX_PATH, "%srecorder.dmp", temp_path);
		printf("%s\r\n", dmp_path);
	}
	

	dump_file(dmp_path, ep);

	return EXCEPTION_EXECUTE_HANDLER;
}

bool APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)exception_handler);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
