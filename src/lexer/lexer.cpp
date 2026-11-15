#include "lexer.h"

#include "tokens.h"
#include "../common/span.h"
#include "../common/utils.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"

#include <cassert>
#include <cstddef>
#include <string_view>
#include <vector>

namespace
{
	struct LexerState
	{
		std::string_view text;
		FileId fileId;
		Diagnostics& diag;
		std::size_t pos{ 0 };
		std::vector<Token> tokens{};
	};

	bool SkipComment(LexerState& state)
	{
		if (state.pos + 1 < state.text.size())
		{
			if (state.text[state.pos + 1] == '/')
			{
				while (state.pos < state.text.size() && state.text[state.pos] != '\n') ++state.pos;
				return true;
			}
		}

		return false;
	}

	void LexIdentifier(LexerState& state)
	{
		std::size_t start{ state.pos };

		while (state.pos < state.text.size() && (IsAlphanumeric(state.text[state.pos]) || state.text[state.pos] == '_')) ++state.pos;

		TokenKind type{ GetKeywordType(state.text.substr(start, state.pos - start)) };
		Span slice{ state.fileId, start, state.pos };

		state.tokens.push_back({ type, slice });
	}

	void LexNumber(LexerState& state)
	{
		std::size_t start{ state.pos };

		while (state.pos < state.text.size() && IsDigit(state.text[state.pos])) ++state.pos;

		if (state.pos < state.text.size() && (IsAlpha(state.text[state.pos]) || state.text[state.pos] == '_'))
		{
			while (state.pos < state.text.size() && (IsAlphanumeric(state.text[state.pos]) || state.text[state.pos] == '_')) ++state.pos;

			Span slice{ state.fileId, start, state.pos };
			state.diag.Error(slice, "encountered malformed integer literal");
			state.tokens.push_back({ TokenKind::Error, slice });
		}
		else
		{
			Span slice{ state.fileId, start, state.pos };
			state.tokens.push_back({ TokenKind::IntLiteral, slice });
		}
	}

	void LexString(LexerState& state)
	{
		std::size_t start{ state.pos };

		++state.pos;
		while (state.pos < state.text.size() && state.text[state.pos] != '"' && state.text[state.pos] != '\n')
		{
			if (state.text[state.pos] == '\\' && state.pos + 1 < state.text.size() && state.text[state.pos + 1] != '\n') ++state.pos;
			++state.pos;
		}

		if (state.pos >= state.text.size() || state.text[state.pos] == '\n')
		{
			Span slice{ state.fileId, start, state.pos };
			if (state.pos > start + 1 && state.text[state.pos - 1] == '\r') --slice.end;
			state.diag.Error(slice, "encountered unterminated string literal");
			state.tokens.push_back({ TokenKind::Error, slice });
		}
		else
		{
			++state.pos;
			state.tokens.push_back({ TokenKind::StringLiteral,{ state.fileId, start, state.pos } });
		}
	}

	void LexErrorCharacter(LexerState& state)
	{
		char c{ state.text[state.pos] };

		Span errSlice{ state.fileId, state.pos++ };

		if (static_cast<unsigned char>(c) >= 0x80)
		{
			while (state.pos < state.text.size() && (state.text[state.pos] & 0xC0) == 0x80) ++state.pos;
		}
		errSlice.end = state.pos;

		state.diag.Error(errSlice, "encountered unknown character");
		state.tokens.push_back({ TokenKind::Error, errSlice });
	}

	void LexPunctuation(LexerState& state)
	{
		char c{ state.text[state.pos] };

		TokenKind type{ TokenKind::Error };
		Span slice{ state.fileId, state.pos, state.pos + 1 };
		switch (c)
		{
		case ';':
			type = TokenKind::Semicolon;
			break;
		case '(':
			type = TokenKind::LParen;
			break;
		case ')':
			type = TokenKind::RParen;
			break;
		case '{':
			type = TokenKind::LBrace;
			break;
		case '}':
			type = TokenKind::RBrace;
			break;
		case ',':
			type = TokenKind::Comma;
			break;
		case '=':
			type = TokenKind::Assign;
			break;
		case '+':
			type = TokenKind::Plus;
			break;
		default:
			LexErrorCharacter(state);
			return;
		}
		++state.pos;

		state.tokens.push_back({ type, slice });
	}
}

std::vector<Token> Lex(const SourceMap& sourceMap, FileId sourceId, Diagnostics& diag)
{
	LexerState state{ sourceMap.File(sourceId).text, sourceId, diag };
	state.tokens.reserve(state.text.size() / 4 + 1);

	while (state.pos < state.text.size())
	{
		std::size_t startPos{ state.pos };
		char c{ state.text[state.pos] };

		if (IsWhitespace(c)) ++state.pos;
		else if (c == '/' && SkipComment(state)) continue;
		else if (IsAlpha(c) || c == '_') LexIdentifier(state);
		else if (IsDigit(c)) LexNumber(state);
		else if (c == '"') LexString(state);
		else LexPunctuation(state);

		assert(state.pos > startPos);
	}

	state.tokens.push_back({ TokenKind::Eof, { sourceId, state.text.size(), state.text.size() } });

	return std::move(state.tokens);
}