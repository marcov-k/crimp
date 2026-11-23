#pragma once

#include <string>
#include <string_view>
#include <vector>

struct Failure
{
	std::string file{};
	int line{ 0 };
	std::string text{};
};

struct CurrentTest
{
	std::string name{};
	std::vector<Failure> failures{};
	bool skipped{ false };
	std::string skipReason{};
};

void SetCurrentTest(CurrentTest* newTest);
CurrentTest* GetCurrentTest();

void ReportFailure(std::string_view file, int line, std::string_view message);

void ReportEqualityFailure(std::string_view file, int line, std::string_view expr1,
	std::string_view expr2, std::string_view formatted1, std::string_view formatted2);

void ReportSkip(std::string_view reason);