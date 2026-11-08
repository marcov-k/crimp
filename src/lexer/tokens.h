#pragma once

#include "../common/span.h"

#include <cstddef>
#include <string_view>

enum class TokenType : uint8_t
{
	SpecialStart,

	Eof,
	Error,

	SpecialEnd,

	AtomStart,

	Identifier,
	IntLiteral,
	StringLiteral,

	AtomEnd,

	KeywordStart,

	KwReturn,
	KwLastStatus,
	KwVoid,

	KeywordEnd,

	OperatorStart,

	Assign,
	Plus,

	OperatorEnd,

	DelimiterStart,

	Semicolon,
	LParen,
	RParen,
	LBrace,
	RBrace,
	Comma,

	DelimiterEnd,
};

inline bool IsSpecial(TokenType type)
{
	return type > TokenType::SpecialStart && type < TokenType::SpecialEnd;
}

inline bool IsAtom(TokenType type)
{
	return type > TokenType::AtomStart && type < TokenType::AtomEnd;
}

inline bool IsKeyword(TokenType type)
{
	return type > TokenType::KeywordStart && type < TokenType::KeywordEnd;
}

inline bool IsOperator(TokenType type)
{
	return type > TokenType::OperatorStart && type < TokenType::OperatorEnd;
}

inline bool IsDelimiter(TokenType type)
{
	return type > TokenType::DelimiterStart && type < TokenType::DelimiterEnd;
}

std::string_view GetSpelling(TokenType token);

TokenType GetKeywordType(std::string_view spelling);

std::string_view GetTokenName(TokenType type);

struct Token
{
	TokenType type{ TokenType::Eof };
	Span span{};
	std::size_t id{};
};