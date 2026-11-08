#include "arena.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace
{
	std::uintptr_t AlignAddress(const char* cursor, std::size_t alignment)
	{
		return (reinterpret_cast<std::uintptr_t>(cursor) + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
	}
}

void* Arena::Alloc(std::size_t size, std::size_t alignment)
{
	assert(alignment % 2 == 0);

	std::uintptr_t end{ reinterpret_cast<std::uintptr_t>(_end) };
	std::uintptr_t cursor{ AlignAddress(_cursor, alignment) };

	if (cursor > end || size > end - cursor)
	{
		std::size_t blockSize{ std::max(BLOCK, size + alignment) };
		_blocks.push_back(std::make_unique_for_overwrite<char[]>(blockSize));

		_cursor = _blocks.back().get();
		_end = _cursor + blockSize;

		cursor = AlignAddress(_cursor, alignment);
	}

	void* result{ reinterpret_cast<void*>(cursor) };
	_cursor = reinterpret_cast<char*>(cursor + size);
	return result;
}