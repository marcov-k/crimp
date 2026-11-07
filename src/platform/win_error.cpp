#include "win_error.h"

#include "utf8_wide.h"

#include <windows.h>

#include <string>

std::string WinErrorMessage(DWORD code)
{
	wchar_t* buf{ nullptr };

	DWORD len{ FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, reinterpret_cast<LPWSTR>(&buf), 0, nullptr) };
	std::string msg{ (len && buf) ? WideToUTF8(std::wstring(buf, len)) : std::string("Unkown error") };

	if (buf) LocalFree(buf);

	while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r' || msg.back() == ' '))
	{
		msg.pop_back();
	}

	return msg + " (error " + std::to_string(code) + ")";
}