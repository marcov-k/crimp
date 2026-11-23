#pragma once

#include <string>

struct RunnerOptions
{
	bool list{ false };
	std::string filter{};
	bool verbose{ false };
};

int RunTests(const RunnerOptions& options);