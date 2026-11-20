#pragma once

#include <string>

class SourceMap;
struct AstProgram;

std::string EmitIL(const SourceMap& source, const AstProgram* program);