#include "utils.h"

#include <algorithm>
#include <cctype>
#include <string>

bool IsNullOrWhitespace(const std::string& s)
{
	return s.empty() || std::all_of(s.begin(), s.end(), [](char c)
		{
			return std::isspace(static_cast<unsigned char>(c));
		});
}