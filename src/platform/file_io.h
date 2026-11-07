#pragma once

#include <filesystem>
#include <string>

bool ReadFile(const std::filesystem::path& p, std::string& out, std::string& error);

bool WriteFile(const std::filesystem::path& p, const std::string& data, std::string& error);

std::filesystem::path MakeTemporaryDirectory(std::string& error);

void PrependToPath(const std::wstring& dir);

std::filesystem::path ResolveTool(const wchar_t* envName, const std::wstring& exeName);