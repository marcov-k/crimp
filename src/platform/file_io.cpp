#include "file_io.h"

#include "utf8_wide.h"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
	std::wstring GetEnv(const wchar_t* name)
	{
		DWORD n{ GetEnvironmentVariableW(name, nullptr, 0) };
		if (n == 0) return {};

		std::wstring v(n, L'\0');
		DWORD got{ GetEnvironmentVariableW(name, v.data(), n) };
		v.resize(got);

		return v;
	}

	std::filesystem::path FindTool(const std::wstring& name)
	{
		DWORD n{ SearchPathW(nullptr, name.c_str(), L".exe", 0, nullptr, nullptr) };
		if (n == 0) return {};

		std::wstring buf(n, L'\0');
		DWORD got{ SearchPathW(nullptr, name.c_str(), L".exe", n, buf.data(), nullptr) };
		buf.resize(got);

		return { buf };
	}
}

bool ReadFile(const std::filesystem::path& p, std::string& out, std::string& error)
{
	std::ifstream f{ p, std::ios::binary | std::ios::ate };
	if (!f)
	{
		error = "cannot open " + PathToUTF8(p);
		return false;
	}

	std::streamsize size{ f.tellg() };
	if (size < 0)
	{
		error = "cannot determine size of " + PathToUTF8(p);
		return false;
	}
	f.seekg(0);

	out.resize(static_cast<size_t>(size));
	if (size > 0 && !f.read(out.data(), size))
	{
		error = "read failed: " + PathToUTF8(p);
		return false;
	}

	return true;
}

bool WriteFile(const std::filesystem::path& p, const std::string& data, std::string& error)
{
	std::ofstream f{ p, std::ios::binary | std::ios::trunc };
	if (!f)
	{
		error = "cannot create " + PathToUTF8(p);
		return false;
	}

	f.write(data.data(), static_cast<std::streamsize>(data.size()));
	f.close();

	if (!f)
	{
		error = "write failed: " + PathToUTF8(p);
		return false;
	}

	return true;
}

std::filesystem::path MakeTemporaryDirectory(std::string& error)
{
	std::error_code ec;
	std::filesystem::path base{ std::filesystem::temp_directory_path(ec) };
	if (ec)
	{
		error = "no temp directory: " + ec.message();
		return {};
	}

	std::filesystem::path dir{ base / ("crimp-" + std::to_string(GetCurrentProcessId()) + "-" +
		std::to_string(GetTickCount64())) };

	std::filesystem::create_directories(dir, ec);
	if (ec)
	{
		error = "cannot create " + PathToUTF8(dir) + ": " + ec.message();
		return {};
	}

	return dir;
}

void PrependToPath(const std::wstring& dir)
{
	DWORD n{ GetEnvironmentVariableW(L"PATH", nullptr, 0) };
	
	std::wstring cur;
	if (n > 0)
	{
		cur.resize(n);
		DWORD got{ GetEnvironmentVariableW(L"PATH", cur.data(), n) };
		cur.resize(got);
	}
	SetEnvironmentVariableW(L"PATH", (dir + L";" + cur).c_str());
}

std::filesystem::path ResolveTool(const wchar_t* envName, const std::wstring& exeName)
{
	std::wstring overridePath{ GetEnv(envName) };
	if (!overridePath.empty())
	{
		std::error_code ec;
		std::filesystem::path p{ overridePath };
		if (std::filesystem::exists(p, ec)) return p;
		return {};
	}
	return FindTool(exeName);
}