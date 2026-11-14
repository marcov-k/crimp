#pragma once

#include "../common/span.h"

#include <vector>

struct Token;
class SourceMap;
class Diagnostics;

[[nodiscard]]
std::vector<Token> Lex(const SourceMap& sourceMap, FileId sourceId, Diagnostics& diag);