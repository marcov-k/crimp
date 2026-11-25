#include "crt_setup.h"

#include "context.h"
#include "error_codes.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <crtdbg.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

namespace
{
	void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, std::uintptr_t)
	{
		if (expression && function && file)
		{
			std::fprintf(stderr, "Invalid parameter in expression '%ls' at line %d in function '%ls' (%ls)\n", expression, line, function, file);
		}
		else
		{
			std::fprintf(stderr, "Invalid parameter at line %d\n", line);
		}

		std::fflush(stdout);
		std::fflush(stderr);
		_exit(CRIMP_TEST_INVALID_CRT_PARAM);
	}

	LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
	{
		DWORD code{ exceptionInfo->ExceptionRecord->ExceptionCode };
		if (const CurrentTest* test = GetCurrentTest())
		{
			std::fprintf(stderr, "Unhandled exception 0x%081X in test %s\n", code, test->name.c_str());
		}
		else
		{
			std::fprintf(stderr, "Unhandled exception 0x%081X\n", code);
		}

		std::fflush(stdout);
		std::fflush(stderr);
		return EXCEPTION_EXECUTE_HANDLER;
	}
}

void SetUpCRT()
{
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);

	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif

	_set_abort_behavior(0, _CALL_REPORTFAULT);
	_set_error_mode(_OUT_TO_STDERR);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	_set_invalid_parameter_handler(InvalidParameterHandler);
	SetUnhandledExceptionFilter(ExceptionHandler);
}