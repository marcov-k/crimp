#include "runner.h"

#include "context.h"
#include "error_codes.h"
#include "registry.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	enum class TestResult : std::uint8_t
	{
		Success,
		Skip,
		Failure,
	};

	std::string_view GetResultSpelling(TestResult r)
	{
		switch (r)
		{
		case TestResult::Success:
			return "Passed";
		case TestResult::Skip:
			return "Skipped";
		case TestResult::Failure:
			return "Failed";
		}

		return {};
	}

	bool MatchesFilter(std::string_view testName, std::string_view filter)
	{
		return testName.find(filter) != std::string::npos;
	}

	bool SelftestAllowed(std::string_view testName, std::string_view filter)
	{
		return testName.find("selftest/") != std::string::npos ? filter.find("selftest/") != std::string::npos : true;
	}

	std::vector<TestEntry> FilterTests(const std::vector<TestEntry>& tests, std::string_view filter)
	{
		std::vector<TestEntry> filtered{};

		std::copy_if(tests.begin(), tests.end(), std::back_inserter(filtered),
			[&filter](const TestEntry& test)
			{
				return MatchesFilter(test.name, filter) && SelftestAllowed(test.name, filter);
			});

		return filtered;
	}

	TestResult RunTest(const TestEntry& test)
	{
		std::printf("[ RUN  ] %s\n", test.name.c_str());
		std::fflush(stdout);
		
		CurrentTest currentTest{ .name = test.name };
		SetCurrentTest(&currentTest);

		test.func();

		SetCurrentTest(nullptr);

		TestResult status;
		if (currentTest.failures.empty())
		{
			if (currentTest.skipped)
			{
				std::printf("[ SKIP ] %s\n", currentTest.skipReason.c_str());
				status = TestResult::Skip;
			}
			else
			{
				std::printf("[  OK  ]\n");
				status = TestResult::Success;
			}
		}
		else
		{
			std::printf("[ FAIL ]\n");
			for (const Failure& failure : currentTest.failures)
			{
				std::printf("%s\n", std::format("\t{}({}): {}", failure.file, failure.line, failure.text).c_str());
			}
			status = TestResult::Failure;
		}

		std::fflush(stdout);
		return status;
	}
}

int RunTests(const RunnerOptions& options)
{
	std::vector<TestEntry> tests{ FilterTests(Registry(), options.filter) };

	if (tests.empty())
	{
		if (options.filter.empty())
		{
			std::fprintf(stderr, "no tests found in registry\n");
		}
		else
		{
			std::fprintf(stderr, "no tests match '%s'\n", options.filter.c_str());
		}
		return CRIMP_TEST_USAGE_ERROR;
	}

	std::sort(tests.begin(), tests.end(),
		[](const TestEntry& a, const TestEntry& b)
		{
			return a.name < b.name;
		});

	if (options.list)
	{
		for (const TestEntry& test : tests)
		{
			std::printf("%s\n", test.name.c_str());
		}

		return CRIMP_TEST_SUCCESS;
	}

	std::vector<TestResult> results{};
	for (const TestEntry& test : tests)
	{
		results.push_back(RunTest(test));
	}

	std::printf("\nSummary:\n");

	std::size_t successes{ 0 }, skips{ 0 }, fails{ 0 };
	std::string failsLog{};
	for (std::size_t i{ 0 }; i < tests.size(); ++i)
	{
		switch (results[i])
		{
		case TestResult::Success:
			++successes;
			break;
		case TestResult::Skip:
			++skips;
			break;
		case TestResult::Failure:
			++fails;
			failsLog.append(std::format("{}: {}\n", tests[i].name, GetResultSpelling(results[i])));
			break;
		}
	}
	std::printf("%zu succeeded, %zu skipped, %zu failed\n", successes, skips, fails);
	std::printf("%s", failsLog.c_str());

	return fails == 0 ? CRIMP_TEST_SUCCESS : CRIMP_TEST_FAILED;
}