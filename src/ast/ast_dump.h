#pragma once

#include "ast.h"
#include "../diagnostics/source_map.h"

#include <string>

std::string DumpAstToString(const SourceMap& source, const AstProgram& program);

void DumpAst(const SourceMap& source, const AstProgram& program);