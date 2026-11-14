#pragma once

#include "../common/span.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

enum class TokenKind : std::uint8_t
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

inline bool IsSpecial(TokenKind type)
{
	return type > TokenKind::SpecialStart && type < TokenKind::SpecialEnd;
}

inline bool IsAtom(TokenKind type)
{
	return type > TokenKind::AtomStart && type < TokenKind::AtomEnd;
}

inline bool IsKeyword(TokenKind type)
{
	return type > TokenKind::KeywordStart && type < TokenKind::KeywordEnd;
}

inline bool IsOperator(TokenKind type)
{
	return type > TokenKind::OperatorStart && type < TokenKind::OperatorEnd;
}

inline bool IsDelimiter(TokenKind type)
{
	return type > TokenKind::DelimiterStart && type < TokenKind::DelimiterEnd;
}

std::string_view GetSpelling(TokenKind token);

TokenKind GetKeywordType(std::string_view spelling);

std::string_view GetTokenName(TokenKind type);

struct Token
{
	TokenKind type{ TokenKind::Eof };
	Span span{};
	std::size_t id{};
};