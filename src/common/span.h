#pragma once

#include <cstddef>
#include <cstdint>

using FileId = std::size_t;

struct Span
{
	FileId file{ 0 };
	std::size_t begin{ 0 };
	std::size_t end{ 0 };
};