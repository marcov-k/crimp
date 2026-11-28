#pragma once

#include "tokens.h"
#include "../diagnostics/source_map.h"

#include <string>
#include <string_view>
#include <vector>

std::string DumpTokensToString(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens);

void DumpTokens(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens);