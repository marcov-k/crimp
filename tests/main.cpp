#include "framework/crt_setup.h"
#include "framework/error_codes.h"
#include "framework/runner.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace
{
	void PrintUsage()
	{
		std::fprintf(stderr, "usage: crimp_tests [--list] [--filter <text>] [--test <name>] [--verbose] [--timeout <seconds>] [--no-timeout]\n");
	}
}

int main(int argc, char** argv)
{
	SetUpCRT();

	RunnerOptions options{};

	for (int i{ 1 }; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--list") == 0) options.list = true;
		else if (std::strcmp(argv[i], "--filter") == 0)
		{
			if (i + 1 >= argc)
			{
				PrintUsage();
				return CRIMP_TEST_USAGE_ERROR;
			}

			options.filter = argv[++i];
		}
		else if (std::strcmp(argv[i], "--test") == 0)
		{
			if (i + 1 >= argc)
			{
				PrintUsage();
				return CRIMP_TEST_USAGE_ERROR;
			}

			options.test = argv[++i];
		}
		else if (std::strcmp(argv[i], "--verbose") == 0) options.verbose = true;
		else if (std::strcmp(argv[i], "--timeout") == 0)
		{
			if (i + 1 >= argc)
			{
				PrintUsage();
				return CRIMP_TEST_USAGE_ERROR;
			}

			if (options.noTimeout) continue;

			try
			{
				double timeout{ std::stod(argv[++i]) };
				if (timeout < 0.0)
				{
					std::fprintf(stderr, "--timeout argument must be positive\n");
					return CRIMP_TEST_USAGE_ERROR;
				}
				options.timeout = timeout;
			}
			catch (const std::exception&)
			{
				std::fprintf(stderr, "--timeout argument must be a number\n");
				return CRIMP_TEST_USAGE_ERROR;
			}
		}
		else if (std::strcmp(argv[i], "--no-timeout") == 0)
		{
			options.timeout = 0.0;
			options.noTimeout = true;
		}
		else { PrintUsage(); return CRIMP_TEST_USAGE_ERROR; }
	}

	return RunTests(options);
}