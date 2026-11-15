#pragma once

#include <string>

class SourceMap;
struct AstProgram;

std::string Emit(const SourceMap& source, const AstProgram* program);