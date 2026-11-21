#pragma once

#include <filesystem>
#include <string>

struct CompileFlags
{
	bool verbose{ false };
	bool keepTemps{ false };
	bool dumpTokens{ false };
	bool dumpAst{ false };
	bool dumpSymbols{ false };
	bool emitSSA{ false };
};

int Compile(const std::filesystem::path& sourcePath, const CompileFlags& flags);