#include "tokens_dump.h"

#include "tokens.h"
#include "../common/utils.h"
#include "../diagnostics/source_map.h"

#include <cstdio>
#include <format>
#include <string>
#include <string_view>
#include <vector>

std::string DumpTokensToString(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens)
{
	std::string_view text{ source.File(fileId).text };

	std::string output{};
	for (const Token& token : tokens)
	{
		LineCol lc{ source.Locate(fileId, token.span.begin) };
		std::string_view lexeme{ text.substr(token.span.begin, token.span.end - token.span.begin) };
		output.append(std::format("{}:{} {} \"{}\"\n", lc.line, lc.col, GetTokenName(token.type), EscapeString(lexeme)));
	}

	return output;
}

void DumpTokens(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens)
{
	std::string output{ DumpTokensToString(source, fileId, tokens) };
	std::fwrite(output.data(), 1, output.size(), stdout);
}