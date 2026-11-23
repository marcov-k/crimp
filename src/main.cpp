#include "driver/driver.h"
#include "platform/file_io.h"
#include "platform/process.h"
#include "platform/startup.h"
#include "platform/utf8_wide.h"

#include <cstdio>
#include <cwchar>
#include <filesystem>

int wmain(int argc, wchar_t** argv)
{
	Initialize();

	if (argc < 2)
	{
		std::fprintf(stderr, "missing source file\n");
		return 1;
	}

	std::filesystem::path sourcePath{ argv[1] };

	CompileFlags flags{};
	for (int i{ 2 }; i < argc; ++i)
	{
		if (std::wcscmp(argv[i], L"--verbose") == 0) flags.verbose = true;
		else if (std::wcscmp(argv[i], L"--keep-temps") == 0) flags.keepTemps = true;
		else if (std::wcscmp(argv[i], L"--dump-tokens") == 0) flags.dumpTokens = true;
		else if (std::wcscmp(argv[i], L"--dump-ast") == 0) flags.dumpAst = true;
		else if (std::wcscmp(argv[i], L"--dump-symbols") == 0) flags.dumpSymbols = true;
		else if (std::wcscmp(argv[i], L"--emit-ssa") == 0) flags.emitSSA = true;
		else { std::fprintf(stderr, "unknown option '%s'\n", WideToUTF8({ argv[i] }).c_str()); return 1; }
	}

	PrependToPath(L"C:\\msys64\\mingw64\\bin");

	return Compile(sourcePath, flags);
}