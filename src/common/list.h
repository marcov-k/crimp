#pragma once

#include <cstddef>

template<typename T>
struct List
{
	T* data{ nullptr };
	std::size_t count{ 0 };

	T* begin() const
	{
		return data;
	}

	T* end() const
	{
		return data + count;
	}
};