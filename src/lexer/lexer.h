#pragma once

#include "tokens.h"
#include "../common/span.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"

#include <vector>

[[nodiscard]]
std::vector<Token> Lex(const SourceMap& sourceMap, FileId sourceId, Diagnostics& diag);