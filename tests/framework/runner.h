#pragma once

#include <string>

struct RunnerOptions
{
	bool list{ false };
	std::string filter{};
	std::string test{};
	bool verbose{ false };
};

int RunTests(const RunnerOptions& options);