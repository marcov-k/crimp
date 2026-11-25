#include "framework/crt_setup.h"
#include "framework/error_codes.h"
#include "framework/runner.h"

#include <cstdio>
#include <cstring>

namespace
{
	void PrintUsage()
	{
		std::fprintf(stderr, "usage: crimp_tests [--list] [--filter <text>] [--verbose]\n");
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
		else if (std::strcmp(argv[i], "--verbose") == 0) options.verbose = true;
		else { PrintUsage(); return CRIMP_TEST_USAGE_ERROR; }
	}

	return RunTests(options);
}