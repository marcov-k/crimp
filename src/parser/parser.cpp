#include "parser.h"

#include "../ast/ast.h"
#include "../common/arena.h"
#include "../common/span.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"
#include "../lexer/tokens.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

struct ParserState
{
	std::vector<Token> tokens{};
	std::size_t cursor{ 0 };
	Diagnostics& diag;
	Arena& arena;
	FileId fileId{};
	std::size_t lastError{};
};

namespace
{
	Token& Peek(std::size_t offset, ParserState& state)
	{
		std::size_t index{ std::min(state.cursor + offset, state.tokens.size()) };
		return state.tokens[index];
	}

	Token& Advance(ParserState& state)
	{
		Token& token{ state.tokens[state.cursor] };

		if (state.cursor + 1 < state.tokens.size())
		{
			++state.cursor;
		}

		return token;
	}

	bool At(TokenType type, const ParserState& state)
	{
		return state.tokens[state.cursor].type == type;
	}

	bool Accept(TokenType type, ParserState& state)
	{
		if (At(type, state))
		{
			++state.cursor;
			return true;
		}

		return false;
	}

	void Expect(TokenType type, ParserState& state, std::string_view message)
	{
		if (!Accept(type, state))
		{
			
		}
	}
}

AstProgram* Parse(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens, Diagnostics& diag, Arena& arena)
{

}