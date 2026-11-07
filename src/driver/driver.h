#pragma once

#include <filesystem>
#include <string>

struct Tools
{
	std::filesystem::path qbe;
	std::filesystem::path cc;
	std::wstring target{ L"amd64_win" };
};

struct CompileFlags
{
	bool verbose{ false };
};

bool BuildExecutable(const std::string& il, const std::filesystem::path& exePath,
	const Tools& tools, bool keepTemps, std::string& error);

int wmain(int /*argc*/, wchar_t** /*argv*/);