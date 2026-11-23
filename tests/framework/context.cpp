#include "context.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	CurrentTest* currentTest{ nullptr };
}

void SetCurrentTest(CurrentTest* newTest)
{
	currentTest = newTest;
}

CurrentTest* GetCurrentTest()
{
	return currentTest;
}

void ReportFailure(std::string_view file, int line, std::string_view message)
{
	assert(currentTest);
	if (!currentTest) return;

	currentTest->failures.push_back({ std::string{ file }, line, std::string{ message } });
}

void ReportEqualityFailure(std::string_view file, int line, std::string_view expr1,
	std::string_view expr2, std::string_view formatted1, std::string_view formatted2)
{
	assert(currentTest);
	if (!currentTest) return;

	ReportFailure(file, line, std::format("CHECK_EQ({}, {}) failed: {} = {}, {} = {}",
		expr1, expr2, expr1, formatted1, expr2, formatted2));
}

void ReportSkip(std::string_view reason)
{
	assert(currentTest);
	if (!currentTest) return;

	currentTest->skipped = true;
	currentTest->skipReason = reason;
}