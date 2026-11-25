#include "../framework/test.h"

#include <cassert>
#include <cstdio>
#include <vector>

TEST(selftest, failing_check)
{
	CHECK(false);
}

TEST(selftest, failing_assert)
{
	assert(false);
}

TEST(selftest, invalid_parameter)
{
	std::printf(nullptr);
}

TEST(selftest, abort_call)
{
	abort();
}

TEST(selftest, null_dereference)
{
	int* ptr{ nullptr };
	*ptr = 1;
}

TEST(selftest, heap_overflow)
{
	std::vector<int> vec{};
	vec.data()[1] = 1;
}