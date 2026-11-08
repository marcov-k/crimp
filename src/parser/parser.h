#pragma once

#include "../ast/ast.h"
#include "../common/arena.h"
#include "../common/span.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"
#include "../lexer/tokens.h"

#include <vector>

AstProgram* Parse(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens, Diagnostics& diag, Arena& arena);