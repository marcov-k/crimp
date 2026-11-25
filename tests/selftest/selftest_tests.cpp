#include "../framework/test.h"

#include <cassert>
#include <cstdio>
#include <vector>

namespace
{
	int Recurse(bool repeat)
	{
		[[maybe_unused]] volatile char buffer[100'000];

		if (repeat) return Recurse(repeat);
		else return 0;
	}
}

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
	volatile int* ptr{ nullptr };
	*ptr = 1;
}

TEST(selftest, heap_overflow)
{
	std::vector<int> vec{4};
	vec.data()[4] = 1;
}

TEST(selftest, division_by_zero)
{
	volatile int zero{ 0 };
	[[maybe_unused]] volatile int res{ 1 / zero };
}

TEST(selftest, stack_overflow)
{
	Recurse(true);
}