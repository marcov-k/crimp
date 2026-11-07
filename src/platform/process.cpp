#include "process.h"

#include "scoped_handle.h"
#include "utf8_wide.h"
#include "win_error.h"

#include <windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace
{
	bool StartProcess(const std::filesystem::path& program,
		const std::vector<std::wstring>& args, HANDLE in, HANDLE out,
		HANDLE errOut, ScopedHandle& process, std::string& error)
	{
		std::wstring cmd{ QuoteArg(program.wstring()) };
		for (const auto& a : args)
		{
			cmd.push_back(L' ');
			cmd += QuoteArg(a);
		}

		STARTUPINFOW si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = in;
		si.hStdOutput = out;
		si.hStdError = errOut;

		_PROCESS_INFORMATION pi{};

		BOOL ok{ CreateProcessW(program.c_str(), cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
			nullptr, nullptr, &si, &pi) };

		if (!ok)
		{
			error = "cannot launch " + PathToUTF8(program) + ": " + WinErrorMessage(GetLastError());
			return false;
		}

		CloseHandle(pi.hThread);
		process.Reset(pi.hProcess);
		return true;
	}

	void WaitAndCollect(ScopedHandle& process, ProcessResult& res)
	{
		WaitForSingleObject(process.Get(), INFINITE);
		DWORD code{ 0 };
		if (GetExitCodeProcess(process.Get(), &code)) res.exitCode = code;
	}

	SECURITY_ATTRIBUTES InheritableAttributes()
	{
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		return sa;
	}

	ScopedHandle OpenNulForStdin()
	{
		SECURITY_ATTRIBUTES sa{ InheritableAttributes() };
		return ScopedHandle{ CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
	}
}

std::wstring QuoteArg(const std::wstring& a)
{
	if (!a.empty() && a.find_first_of(L" \t\n\v\"") == std::wstring::npos)
	{
		return a;
	}

	std::wstring out{ L"\"" };

	size_t backslashes{ 0 };
	for (wchar_t c : a)
	{
		if (c == L'\\')
		{
			++backslashes;
		}
		else if (c == L'"')
		{
			out.append(backslashes * 2 + 1, L'\\');
			out.push_back(L'"');
			backslashes = 0;
		}
		else
		{
			out.append(backslashes, L'\\');
			out.push_back(c);
			backslashes = 0;
		}
	}
	out.append(backslashes * 2, L'\\');
	out.push_back(L'"');

	return out;
}

ProcessResult RunLogged(const std::filesystem::path& program,
	const std::vector<std::wstring>& args, const std::filesystem::path& logPath)
{
	ProcessResult res;
	SECURITY_ATTRIBUTES sa{ InheritableAttributes() };

	ScopedHandle log{ CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };

	if (!log.Valid())
	{
		res.error = "cannot create log file " + PathToUTF8(logPath) + ": " + WinErrorMessage(GetLastError());
		return res;
	}

	ScopedHandle nul{ OpenNulForStdin() };
	if (!nul.Valid())
	{
		res.error = "cannot open NUL device: " + WinErrorMessage(GetLastError());
		return res;
	}

	ScopedHandle proc;
	if (!StartProcess(program, args, nul.Get(), log.Get(), log.Get(), proc, res.error))
	{
		return res;
	}
	res.launched = true;

	log.Reset();
	nul.Reset();

	WaitAndCollect(proc, res);

	return res;
}

ProcessResult RunCaptured(const std::filesystem::path& program,
	const std::vector<std::wstring>& args, std::string& output)
{
	ProcessResult res;
	SECURITY_ATTRIBUTES sa{ InheritableAttributes() };

	HANDLE r{ nullptr }, w{ nullptr };
	if (!CreatePipe(&r, &w, &sa, 0))
	{
		res.error = "CreatePipe failed: " + WinErrorMessage(GetLastError());
		return res;
	}

	ScopedHandle readEnd{ r }, writeEnd{ w };
	SetHandleInformation(readEnd.Get(), HANDLE_FLAG_INHERIT, 0);

	ScopedHandle nul{ OpenNulForStdin() };
	if (!nul.Valid())
	{
		res.error = "cannot open NUL device: " + WinErrorMessage(GetLastError());
		return res;
	}

	ScopedHandle proc;
	if (!StartProcess(program, args, nul.Get(), writeEnd.Get(), writeEnd.Get(), proc, res.error))
	{
		return res;
	}
	res.launched = true;

	writeEnd.Reset();
	nul.Reset();

	char buf[4096];
	DWORD got{ 0 };
	while (ReadFile(readEnd.Get(), buf, sizeof(buf), &got, nullptr) && got > 0)
	{
		output.append(buf, got);
	}
	
	WaitAndCollect(proc, res);

	return res;
}