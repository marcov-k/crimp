#pragma once

#include "scoped_handle.h"

#include <windows.h>

#include <filesystem>
#include <string>
#include <vector>

struct ProcessResult
{
	bool launched{ false };
	unsigned long exitCode{ 0 };
	std::string error;
};

std::wstring QuoteArg(const std::wstring& a);

ProcessResult RunLogged(const std::filesystem::path& program,
	const std::vector<std::wstring>& args, const std::filesystem::path& logPath);

ProcessResult RunCaptured(const std::filesystem::path& program,
	const std::vector<std::wstring>& args, std::string& output);