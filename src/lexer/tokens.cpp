#include "tokens.h"

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
	static_assert(std::size(spellings) == static_cast<size_t>(TokenType::DelimiterEnd) + 1);

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
	static_assert(std::size(tokenNames) == static_cast<size_t>(TokenType::DelimiterEnd) + 1);
}

std::string_view GetSpelling(TokenType token)
{
	return spellings[static_cast<size_t>(token)];
}

TokenType GetKeywordType(std::string_view spelling)
{
	for (size_t i{ static_cast<size_t>(TokenType::KeywordStart) + 1 }; i < static_cast<size_t>(TokenType::KeywordEnd); ++i)
	{
		if (spelling == spellings[i])
		{
			return static_cast<TokenType>(i);
		}
	}

	return TokenType::Identifier;
}

std::string_view GetTokenName(TokenType type)
{
	return tokenNames[static_cast<size_t>(type)];
}