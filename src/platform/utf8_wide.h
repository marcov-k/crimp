#pragma once

#include <filesystem>
#include <string>

std::wstring UTF8ToWide(const std::string& s);

std::string WideToUTF8(const std::wstring& w);

std::string PathToUTF8(const std::filesystem::path& p);