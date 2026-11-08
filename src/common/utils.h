#pragma once

#include <algorithm>
#include <cctype>
#include <string>

bool IsNullOrWhitespace(const std::string& s);

inline bool IsWhitespace(char c)
{
	return std::isspace(static_cast<unsigned char>(c));
}

inline bool IsAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsDigit(char c)
{
	return c >= '0' && c <= '9';
}

inline bool IsAlphanumeric(char c)
{
	return IsAlpha(c) || IsDigit(c);
}