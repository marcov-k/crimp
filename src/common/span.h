#pragma once

#include <cstdint>

using FileId = size_t;

struct Span
{
	FileId file{ 0 };
	size_t begin{ 0 };
	size_t end{ 0 };
};