#include "lexer.h"

#include "tokens.h"
#include "../common/span.h"
#include "../common/utils.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"

#include <cstddef>
#include <string_view>
#include <vector>

std::vector<Token> Lex(const SourceMap& sourceMap, FileId sourceId, Diagnostics& diag)
{
	const SourceFile& file{ sourceMap.File(sourceId) };
	std::string_view text{ file.text };
	std::vector<Token> tokens{};

	std::size_t textSize{ file.text.size() };
	tokens.reserve(textSize / 4 + 1);

	std::size_t pos{ 0 };
	while (pos < textSize)
	{
		char c{ text[pos] };
		if (IsWhitespace(c))
		{
			++pos;
			continue;
		}

		if (c == '/')
		{
			if (pos + 1 < textSize)
			{
				if (text[pos + 1] == '/')
				{
					while (pos < textSize && text[pos] != '\n') ++pos;
					continue;
				}
			}
		}

		if (IsAlpha(c) || c == '_')
		{
			std::size_t start{ pos };

			while (pos < textSize && (IsAlphanumeric(text[pos]) || text[pos] == '_')) ++pos;

			TokenType type{ GetKeywordType(text.substr(start, pos - start)) };
			Span slice{ sourceId, start, pos };

			tokens.push_back({ type, slice });

			continue;
		}

		if (IsDigit(c))
		{
			std::size_t start{ pos };

			while (pos < textSize && IsDigit(text[pos])) ++pos;

			if (pos < textSize && (IsAlpha(text[pos]) || text[pos] == '_'))
			{
				while (pos < textSize && (IsAlphanumeric(text[pos]) || text[pos] == '_')) ++pos;

				Span slice{ sourceId, start, pos };
				diag.Error(slice, "encountered malformed integer literal");
				tokens.push_back({ TokenType::Error, slice });
			}
			else
			{
				Span slice{ sourceId, start, pos };
				tokens.push_back({ TokenType::IntLiteral, slice });
			}

			continue;
		}

		if (c == '"')
		{
			std::size_t start{ pos };

			++pos;
			while (pos < textSize && text[pos] != '"' && text[pos] != '\n')
			{
				if (text[pos] == '\\')
				{
					if (pos + 1 < textSize && text[pos + 1] != '\n') ++pos;
				}
				++pos;
			}

			if (pos >= textSize || text[pos] == '\n')
			{
				Span slice{ sourceId, start, pos };
				if (pos > start + 1 && text[pos - 1] == '\r')
				{
					--slice.end;
				}
				diag.Error(slice, "encountered unterminated string literal");
				tokens.push_back({ TokenType::Error, slice });
			}
			else
			{
				++pos;
				tokens.push_back({ TokenType::StringLiteral, { sourceId, start, pos } });
			}

			continue;
		}

		TokenType type{ TokenType::Error };
		Span slice{ sourceId, pos, pos + 1 };
		switch (c)
		{
		case ';':
			type = TokenType::Semicolon;
			break;
		case '(':
			type = TokenType::LParen;
			break;
		case ')':
			type = TokenType::RParen;
			break;
		case '{':
			type = TokenType::LBrace;
			break;
		case '}':
			type = TokenType::RBrace;
			break;
		case ',':
			type = TokenType::Comma;
			break;
		case '=':
			type = TokenType::Assign;
			break;
		case '+':
			type = TokenType::Plus;
			break;
		default:
		{
			Span errSlice;
			errSlice.file = sourceId;
			errSlice.begin = pos++;

			if (static_cast<unsigned char>(c) < 0x80)
			{
				errSlice.end = pos;
			}
			else
			{
				while (pos < textSize && (text[pos] & 0xC0) == 0x80) ++pos;
				errSlice.end = pos;
			}

			diag.Error(errSlice, "encountered unknown character");
			tokens.push_back({ TokenType::Error, errSlice });

			continue;
		}
		}
		++pos;

		tokens.push_back({ type, slice });
	}

	tokens.push_back({ TokenType::Eof, { sourceId, textSize, textSize } });

	return tokens;
}