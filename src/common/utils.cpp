#include "utils.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

bool IsNullOrWhitespace(const std::string& s)
{
	return s.empty() || std::all_of(s.begin(), s.end(), [](char c)
		{
			return std::isspace(static_cast<unsigned char>(c));
		});
}

std::string EscapeString(std::string_view s)
{
	static constexpr char hex[] = "0123456789abcdef";

	std::string out;
	out.reserve(s.size());

	for (char ch : s)
	{
		unsigned char b{ static_cast<unsigned char>(ch) };

		switch (b)
		{
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '"':
			out += "\\\"";
			break;
		default:
			if (b < 0x20 || b >= 0x7F)
			{
				out += "\\x";
				out += hex[b >> 4];
				out += hex[b & 0x0F];
			}
			else
			{
				out += ch;
			}
			break;
		}
	}

	return out;
}