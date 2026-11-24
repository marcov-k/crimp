#pragma once

#include <string>
#include <string_view>

inline bool StringContains(std::string_view haystack, std::string_view needle)
{
	return haystack.find(needle) != std::string::npos;
}