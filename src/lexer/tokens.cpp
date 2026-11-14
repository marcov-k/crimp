#include "tokens.h"

#include <cstddef>
#include <string_view>

namespace
{
	constexpr std::string_view spellings[] =
	{
		"", // SpecialStart

		"end of file",
		"error",

		"", // SpecialEnd

		"", // AtomStart

		"identifier",
		"integer literal",
		"string literal",

		"", // AtomEnd

		"", // KeywordStart

		"return",
		"LastStatus",
		"void",

		"", // KeywordEnd

		"", // OperatorStart

		"=",
		"+",

		"", // OperatorEnd

		"", // DelimiterStart

		";",
		"(",
		")",
		"{",
		"}",
		",",

		"", // DelimiterEnd
	};
	static_assert(std::size(spellings) == static_cast<std::size_t>(TokenKind::DelimiterEnd) + 1);

	constexpr std::string_view tokenNames[] =
	{
		"SpecialStart",

		"Eof",
		"Error",

		"SpecialEnd",

		"AtomStart",

		"Identifier",
		"IntLiteral",
		"StringLiteral",

		"AtomEnd",

		"KeywordStart",

		"KwReturn",
		"KwLastStatus",
		"KwVoid",

		"KeywordEnd",

		"OperatorStart",

		"Assign",
		"Plus",

		"OperatorEnd",

		"DelimiterStart",

		"Semicolon",
		"LParen",
		"RParen",
		"LBrace",
		"RBrace",
		"Comma",

		"DelimiterEnd",
	};
	static_assert(std::size(tokenNames) == static_cast<std::size_t>(TokenKind::DelimiterEnd) + 1);
}

std::string_view GetSpelling(TokenKind token)
{
	return spellings[static_cast<std::size_t>(token)];
}

TokenKind GetKeywordType(std::string_view spelling)
{
	for (std::size_t i{ static_cast<std::size_t>(TokenKind::KeywordStart) + 1 }; i < static_cast<std::size_t>(TokenKind::KeywordEnd); ++i)
	{
		if (spelling == spellings[i])
		{
			return static_cast<TokenKind>(i);
		}
	}

	return TokenKind::Identifier;
}

std::string_view GetTokenName(TokenKind type)
{
	return tokenNames[static_cast<std::size_t>(type)];
}