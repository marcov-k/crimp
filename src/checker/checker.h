#pragma once

struct AstProgram;
class SourceMap;
class Diagnostics;
class Arena;

void Analyze(AstProgram* program, const SourceMap& source, Diagnostics& diag, Arena& arena);