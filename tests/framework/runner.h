#pragma once

#include <string>

struct RunnerOptions
{
	bool list{ false };
	std::string filter{};
	std::string test{};
	bool verbose{ false };
	double timeout{ 10.0 }; // in seconds
	bool noTimeout{ false }; // overrides timeout
};

int RunTests(const RunnerOptions& options);