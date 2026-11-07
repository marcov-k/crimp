#include "utf8_wide.h"

#include <windows.h>

#include <filesystem>
#include <string>

std::wstring UTF8ToWide(const std::string& s)
{
	if (s.empty()) return {};

	int n{ MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0) };
	if (n <= 0) return {};

	std::wstring w(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), w.data(), n);

	return w;
}

std::string WideToUTF8(const std::wstring& w)
{
	if (w.empty()) return {};

	int n{ WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr) };
	if (n <= 0) return {};

	std::string s(static_cast<size_t>(n), '\0');
	WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);

	return s;
}

std::string PathToUTF8(const std::filesystem::path& p)
{
	return WideToUTF8(p.wstring());
}