#pragma once

#include <string>

class SourceMap;
struct AstProgram;

std::string DumpAstToString(const SourceMap& source, const AstProgram* program, bool typed = false);

void DumpAst(const SourceMap& source, const AstProgram* program, bool typed = false);