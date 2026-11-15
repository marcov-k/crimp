#pragma once

#include <string>

struct AstProgram;

std::string DumpSymbolsToString(const AstProgram* program);

void DumpSymbols(const AstProgram* program);