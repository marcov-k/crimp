#pragma once

#include <filesystem>
#include <string>

struct Tools
{
	std::filesystem::path qbe;
	std::filesystem::path cc;
	std::wstring target{ L"amd64_win" };
};

int wmain(int argc, wchar_t** argv);