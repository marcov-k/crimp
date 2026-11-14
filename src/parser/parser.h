#pragma once

#include "../common/span.h"

#include <vector>

struct AstProgram;
struct Token;
class Diagnostics;
class Arena;

AstProgram* Parse(FileId fileId, const std::vector<Token>& tokens, Diagnostics& diag, Arena& arena);